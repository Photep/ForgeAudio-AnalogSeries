#pragma once
// src/dsp/ClockTracker.hpp
//
// Clock-tracking FSM (EMA period smoothing + outlier rejection + 3-state machine
// FREE/ACQUIRING/LOCKED + fast-track re-acquisition), lifted VERBATIM from
// src/AnalogLFO.cpp L407-544 (processClockInput) and L40 (ClockState enum).
//
// Injection (Pattern 2): the two Rack-array reads in the inline code —
//   the CLK_INPUT isConnected() read   (AnalogLFO.cpp:408)
//   the CLK_INPUT getVoltage() read    (AnalogLFO.cpp:446)
// become the injected params `connected` / `clkV`. The ratio index, which the
// inline code reads via paramQuantities[RATE_PARAM]->getScaledValue()
// (AnalogLFO.cpp:509-512), is computed by the shell / RatioTable and passed in
// as `ratioIdx`.
//
// Separation of concerns: the inline FSM performed the phase reset + crossfade
// capture + resetBlanking.trigger directly (L456-460, L528-532). Those belong to
// LfoCore orchestration, so ClockTracker only SIGNALS "reset wanted this edge"
// via Result::resetWanted (the first-edge phase reset is signalled too). Display
// atomics (AnalogLFO.cpp:421, 441, 484, 541) STAY IN THE SHELL — not here.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

#include <cmath>
#include <algorithm>

#include "dsp/RackCompat.hpp"   // forge::SchmittTrigger, forge::Timer
#include "dsp/RatioTable.hpp"   // forge::RATIO_TABLE (division-aware reset check)

namespace forge {

// Clock tracking state machine (AnalogLFO.cpp:40).
enum ClockState { FREE = 0, ACQUIRING = 1, LOCKED = 2 };

struct ClockTracker {
	// --- FSM state (lifted from the AnalogLFO members) ---
	SchmittTrigger clockTrigger;
	Timer clockTimer;
	float smoothedPeriod = 0.f;
	float lastSmoothedPeriod = 0.f;
	int clockEdgeCount = 0;
	ClockState clockState = FREE;
	bool prevClkConnected = false;
	int clockBeatCount = 0;   // counts clock edges within one LFO cycle
	int prevRatioIdx = -1;    // tracks ratio changes to reset beat counter

	struct Result {
		ClockState state = FREE;
		float smoothedPeriod = 0.f;
		bool edgeFired = false;     // a clock edge was detected this sample
		bool resetWanted = false;   // LfoCore should reset phase + capture crossfade this sample
	};

	// step() — verbatim port of processClockInput(sampleTime) (AnalogLFO.cpp:407-544).
	// `clkV`/`connected` replace the CLK_INPUT array reads; `ratioIdx` replaces the
	// paramQuantities[RATE_PARAM] scaled-value read. Returns the FSM state, smoothed
	// period, and the per-edge reset signal for LfoCore.
	Result step(float clkV, float dt, bool connected, int ratioIdx) {
		Result r;
		r.state = clockState;
		r.smoothedPeriod = smoothedPeriod;

		bool clkConnected = connected;

		// Instant revert on cable disconnect (AnalogLFO.cpp:411-425)
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
			}
			prevClkConnected = false;
			r.state = clockState;
			r.smoothedPeriod = smoothedPeriod;
			return r;
		}
		prevClkConnected = true;

		// Accumulate time (AnalogLFO.cpp:429)
		clockTimer.process(dt);

		// Timeout check (AnalogLFO.cpp:432-443)
		if (clockState != FREE && smoothedPeriod > 0.f) {
			float timeout = std::fmax(1.0f, std::fmin(3.0f * smoothedPeriod, 5.0f));
			if (clockTimer.getTime() > timeout) {
				lastSmoothedPeriod = smoothedPeriod;
				clockState = FREE;
				clockEdgeCount = 0;
				clockBeatCount = 0;
				smoothedPeriod = 0.f;
				clockTimer.reset();
			}
		}

		// Edge detection (AnalogLFO.cpp:446-447)
		float clkVoltage = clkV;
		if (clockTrigger.process(clkVoltage, 0.1f, 1.0f)) {
			r.edgeFired = true;
			float rawPeriod = clockTimer.getTime();
			clockTimer.reset();
			clockEdgeCount++;

			if (clockEdgeCount == 1) {
				// First edge: always reset phase and enter ACQUIRING (AnalogLFO.cpp:452-461)
				clockState = ACQUIRING;
				clockBeatCount = 0;
				r.resetWanted = true;   // shell: crossfadeFrom=lastOutputVoltage; crossfadeProgress=0; phase=0; resetBlanking.trigger(0.001f)
			}
			else if (rawPeriod > 0.001f) {
				// Second edge onward: we have a period measurement

				// Outlier rejection (LOCKED state only, per CLK-05) (AnalogLFO.cpp:466-472)
				if (clockState == LOCKED && smoothedPeriod > 0.f) {
					bool isOutlier = (rawPeriod > 3.0f * smoothedPeriod) ||
					                 (rawPeriod < smoothedPeriod / 3.0f);
					if (isOutlier) {
						// Silently discard (AnalogLFO.cpp:470)
						r.state = clockState;
						r.smoothedPeriod = smoothedPeriod;
						return r;
					}
				}

				// Fast-track re-acquisition check (AnalogLFO.cpp:477-486)
				if (clockState == ACQUIRING && clockEdgeCount == 2 && lastSmoothedPeriod > 0.f) {
					float ratio = rawPeriod / lastSmoothedPeriod;
					if (ratio > 0.8f && ratio < 1.2f) {
						smoothedPeriod = lastSmoothedPeriod;
						smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
						clockState = LOCKED;
					}
				}

				// EMA smoothing (per CLK-03) (AnalogLFO.cpp:489-493)
				if (smoothedPeriod <= 0.f) {
					smoothedPeriod = rawPeriod;  // First measurement: snap
				} else {
					smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
				}

				// State transition: ACQUIRING -> LOCKED after 4 consistent edges (AnalogLFO.cpp:496-501)
				if (clockState == ACQUIRING && clockEdgeCount >= 4) {
					clockState = LOCKED;
				}

				// Division-aware phase reset (RATE-04) (AnalogLFO.cpp:504-533)
				clockBeatCount++;

				// Determine current ratio for division check. The inline code computed
				// currentRatioIdx from the rate-knob scaled value (L507-512); here it is
				// the injected `ratioIdx` (-1 when not clocked-ready).
				int currentRatioIdx = -1;
				if ((clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f) {
					currentRatioIdx = clampi(ratioIdx, 0, 14);
				}

				// Reset beat counter if ratio changed (AnalogLFO.cpp:515-518)
				if (currentRatioIdx != prevRatioIdx && prevRatioIdx >= 0) {
					clockBeatCount = 1;
				}
				prevRatioIdx = currentRatioIdx;

				// Division-aware reset decision (AnalogLFO.cpp:520-524). Delegate to
				// forge::shouldReset so the Phase-23 BEATS_PER_ALIGN fix has a single
				// home — re-implementing it here would silently split the clock FSM
				// from the table when P23 patches shouldReset (CR-03).
				bool reset = forge::shouldReset(currentRatioIdx, clockBeatCount);

				if (reset) {
					// Shell performs: crossfadeFrom=lastOutputVoltage; crossfadeProgress=0;
					// phase=0; resetBlanking.trigger(0.001f). (AnalogLFO.cpp:526-533)
					r.resetWanted = true;
					clockBeatCount = 0;
				}
			}
			// NOTE: the LOCKED-edge display badge counter (AnalogLFO.cpp:540-542) STAYS IN
			// THE SHELL; the shell can re-derive it from r.edgeFired + r.state == LOCKED.
		}

		r.state = clockState;
		r.smoothedPeriod = smoothedPeriod;
		return r;
	}
};

} // namespace forge
