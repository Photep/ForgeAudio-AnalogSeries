# Project Research Summary

**Project:** Forge Audio - Analog Series (VCV Rack LFO + VCO)
**Domain:** Analog-modeled virtual synthesizer oscillator modules
**Researched:** 2026-02-25
**Confidence:** MEDIUM-HIGH

## Executive Summary

The Forge Audio Analog Series is a pair of VCV Rack 2 modules (LFO and VCO) built around a three-knob analog engine: morph (continuous waveform sweep sine-tri-saw-square), character (crossfade from digital perfection to classic synth references like Minimoog saw and Roland square), and drift (layered analog imperfections). This is a well-understood problem domain. VCV Rack 2's C++17 SDK provides all necessary DSP primitives (MinBlepGenerator, SchmittTrigger, exp2_taylor5, TRCFilter), NanoVG for custom display rendering, and a stable build system (GNU Make + plugin.mk). No external libraries are needed. The existing POC validates the core build pipeline and basic waveform generation. The architecture cleanly separates a portable AnalogEngine (pure C++ DSP with no SDK dependencies) from VCV Rack host modules, with a lock-free double buffer bridging the audio and GUI threads for the waveform display.

The recommended approach is LFO-first development. The LFO validates the entire three-knob engine at sub-audio rates where antialiasing is irrelevant, then the VCO reuses the same engine and adds audio-rate concerns (polyBLEP antialiasing, V/Oct tracking, hard sync, through-zero FM). The morph architecture uses linear crossfade between adjacent waveform shapes along a sine-tri-saw-square chain. Character modeling targets specific classic synths per waveform (Minimoog saw via exponential ramp curvature and soft reset, Roland square via sigmoid edge softening and duty asymmetry, Moog/Prophet triangle via rounded peaks, and analog sine via residual 1-3% THD). Drift uses layered independent noise sources at different timescales (Ornstein-Uhlenbeck pitch drift, phase jitter, DC offset wander, pitch slew, per-instance component spread) rather than a single noise generator.

The primary risks are: morph discontinuities producing clicks during sweeps (prevent with phase-aligned waveform definitions and derivative-continuous transitions), incorrect polyBLEP implementation causing aliasing artifacts in the VCO (prevent with known-correct reference implementation and spectral verification), analog character modeling falling into an uncanny valley (prevent by targeting specific measured synth characteristics and validating with A/B listening tests), and display rendering blocking the audio thread (prevent with lock-free double buffer architecture from day one). Research found genuine market whitespace -- no existing VCV Rack module combines continuous four-shape morphing, specific classic synth reference modeling, and a multi-dimensional imperfection system.

## Key Findings

### Recommended Stack

The stack is VCV Rack 2 SDK exclusively -- no external libraries. The SDK provides everything needed: C++17 compiled against plugin.mk, MinBlepGenerator for antialiasing, exp2_taylor5 for pitch conversion, SchmittTrigger for sync, TRCFilter/ExponentialFilter for CV smoothing, and NanoVG for custom display drawing. The DSP engines (morph, character, drift) are hand-rolled pure C++ with no SDK dependencies, making them testable in isolation and potentially portable.

**Core technologies:**
- **VCV Rack 2 SDK (2.5.x):** Plugin host framework -- the only option for VCV modules
- **C++17 via GNU Make + plugin.mk:** Build system -- do not fight this with CMake
- **PolyBLEP (4th-order):** Primary antialiasing for VCO waveform discontinuities (minBLEP available in SDK as upgrade path)
- **NanoVG (SDK-bundled):** Waveform display rendering via TransparentWidget
- **Custom AnalogEngine (hand-rolled):** Morph + character + drift stages as portable C++
- **Double-precision phase accumulator:** Prevents LFO stall at ultra-low rates and VCO pitch drift at high frequencies

**Critical version requirement:** C++17 only -- C++20 breaks cross-platform SDK builds.

### Expected Features

**Must have (table stakes):**
- Four core waveforms with continuous morph sweep between them
- Antialiased audio output for VCO (polyBLEP, 4th order)
- V/Oct pitch input (VCO), rate knob (LFO)
- FM input, hard sync (VCO), reset/sync (LFO)
- CV inputs for morph, character, and drift
- Real-time waveform display with phase-tracking dot
- Analog character control (the core product promise)
- Pitch drift and HF rolloff (most perceptible analog imperfections)
- Bipolar +/-5V output, inverted output
- Reasonable CPU (<1% single core per instance)
- Right-click context menu for secondary settings

**Should have (differentiators):**
- Three-knob analog engine as independent CV-controllable axes (unique in VCV Rack)
- Named classic synth references per waveform shape (Minimoog, Roland, Moog/Prophet)
- Component spread giving each instance unique "personality"
- Through-zero FM (VCO) for clean, musical FM timbres
- Oversampling option (Off/2x/4x) via context menu

**Defer (v2+):**
- MinBLEP upgrade (only if polyBLEP aliasing complaints arise)
- Phase distortion (interesting but adds scope; evaluate after core is solid)
- Polyphonic operation (16x CPU, complex display, mono is the modular norm)
- Tracking error (implement toggle but default OFF; subtle specialist feature)
- Waveform bleed in morph zones (subtle polish, low priority)

### Architecture Approach

Three-layer separation: Host Module (VCV Rack Module subclass handling I/O, phase accumulation, and module-specific features), AnalogEngine (pure C++ class with no SDK dependencies containing MorphStage, CharacterStage, and DriftProcessor), and WaveformDisplay (NanoVG TransparentWidget reading a lock-free snapshot buffer). The engine takes normalized inputs (phase 0-1, params 0-1) and returns normalized output (-1 to +1). The host handles all VCV-specific voltage conventions.

**Major components:**
1. **AnalogEngine** -- Orchestrates MorphStage -> CharacterStage -> DriftProcessor pipeline; owns display buffer
2. **MorphStage** -- Linear crossfade between adjacent waveforms across three segments (sine-tri, tri-saw, saw-square); stateless pure math
3. **CharacterStage** -- Per-shape analog reference generators (mathematical deformation, not wavetables); crossfades digital output toward analog reference
4. **DriftProcessor** -- Six independent noise sources (pitch drift, phase jitter, component spread, DC offset, HF rolloff, pitch slew); xorshift PRNG with filtered noise shaping
5. **WaveformDisplay** -- 256-sample lock-free double buffer; TransparentWidget (not FramebufferWidget) since phase dot animates every frame
6. **Host Modules** (ForgeAnalogLFO, ForgeAnalogVCO) -- Phase accumulation, I/O routing, antialiasing (VCO only), sync/FM handling

### Critical Pitfalls

1. **Morph click artifacts** -- Naive crossfade between waveforms with different derivative profiles produces clicks. Prevent with phase-aligned definitions (all shapes share zero-crossings at phase 0 and 0.5), sine-tri-saw-square ordering, and parameter slew limiting (1-5ms lowpass on morph CV). Must be addressed in the first phase.

2. **Incorrect polyBLEP aliasing** -- Wrong fractional delay calculation, wrong discontinuity height, or missing edges on square waves makes aliasing worse, not better. Start with the canonical 2-sample polyBLEP implementation and verify spectrally (10kHz saw at 44.1kHz, check for mirror frequencies). Always address in the core oscillator, not bolted on later.

3. **Display blocking audio thread** -- Any mutex the audio thread waits on causes dropouts. Users report "module sounds great but clicks when I look at it." Use lock-free double buffer with atomic index swap from day one. This is an architectural decision, not a feature.

4. **Analog character uncanny valley** -- Half-modeled analog character sounds "wrong" rather than "warm." Prevent by targeting specific measured synth characteristics (Minimoog capacitor discharge, Roland slew rate), calibrating in objective units (cents of drift, % THD), and A/B testing against reference recordings. Less is more -- real Minimoog oscillators are surprisingly clean.

5. **Phase accumulator precision** -- Float32 phase accumulator stalls LFO below ~0.01Hz and drifts VCO at high pitches. Use double-precision. Trivial to prevent, catastrophic if caught late.

## Implications for Roadmap

Based on research, the project naturally divides into 5 phases following a strict dependency chain. The LFO-first strategy is strongly validated: it proves the entire engine without audio-rate complexity.

### Phase 1: Foundation -- Morph Engine and Display
**Rationale:** MorphStage is the foundation everything else builds on. WaveformDisplay built early provides visual feedback that accelerates all subsequent development. Lock-free display architecture must be designed from the start (Pitfall 5).
**Delivers:** Working LFO module with four-shape morph sweep and real-time waveform display with phase dot. No analog character yet, but the core interaction loop is complete.
**Addresses:** Core waveform generation, morph sweep, display with phase dot, bipolar output, inverted output, reset/sync input, FM input, rate knob.
**Avoids:** Morph discontinuities (P1) via phase-aligned waveform definitions; display threading (P5) via lock-free double buffer; phase precision (P9) via double accumulator; memory allocation (P6) via pre-allocated buffers; voltage standards (P14) via defined constants; sample rate handling (P16) via onSampleRateChange().

### Phase 2: Character Engine -- Analog Reference Modeling
**Rationale:** Character defines the target sound that makes this module worth using. It must come before drift because character is the "what it sounds like" and drift is the "how it behaves." Tuning drift without knowing the character target is working blind.
**Delivers:** Character knob crossfading from digital to analog references for all four waveform shapes. HF rolloff (pitch-tracking lowpass). The module now sounds distinctly different from Fundamental VCO.
**Addresses:** Minimoog saw reference, Roland square reference, Moog/Prophet triangle reference, analog sine with residual THD, HF rolloff, character CV input.
**Avoids:** Uncanny valley (P7) via specific synth targets with measured characteristics; testing by ear only (P15) via quantitative THD and spectral targets.

### Phase 3: Drift Engine -- Analog Imperfections
**Rationale:** Drift is independent of character and adds the "living, breathing" quality. Built on top of a solid character engine so the drift modulates a known-good sound.
**Delivers:** Full three-knob LFO with pitch drift, phase jitter, DC offset, pitch slew, component spread. Each module instance sounds slightly unique. The core product promise is now complete.
**Addresses:** Drift knob with all six imperfection sources, drift CV input, component spread via per-instance seed, state serialization for recall.
**Avoids:** Bad drift modeling (P8) via layered 1/f noise sources at different timescales (not white noise); serialization loss (P11) via versioned JSON with deterministic seed storage.

### Phase 4: VCO Core -- Audio-Rate Antialiasing
**Rationale:** VCO reuses the entire AnalogEngine from the LFO and adds audio-rate concerns. PolyBLEP antialiasing must be morph-aware (discontinuity structure changes with morph position), which is the trickiest implementation challenge in the project.
**Delivers:** ForgeAnalogVCO with V/Oct tracking, morph-aware polyBLEP antialiasing, hard sync with sub-sample BLEP compensation. Full three-knob engine inherited from LFO.
**Addresses:** V/Oct input, polyBLEP antialiasing (4th order), hard sync with antialiased sync discontinuities, coarse/fine tune, oversampling option (Off/2x/4x).
**Avoids:** Wrong BLEP (P2) via canonical implementation with spectral verification; hard sync aliasing (P4) via fractional sample timing and BLEP at sync points; wasted oversampling (P10) via surgical application only where needed.

### Phase 5: VCO Advanced -- FM and Polish
**Rationale:** Through-zero FM is a premium differentiator that builds on top of the stable VCO. It has its own aliasing challenges (phase increment can skip cycles) that should not compromise the base oscillator quality.
**Delivers:** Through-zero FM, DC blocking filter for VCO output, final CPU optimization, oversampling refinement.
**Addresses:** Through-zero FM with soft-limited phase increment, FM depth attenuverter, DC offset filtering (VCO only), phase distortion (stretch goal).
**Avoids:** TZFM catastrophic aliasing (P3) via surgical oversampling on FM path and soft-limiting at extreme indices.

### Phase Ordering Rationale

- **Strict dependency chain:** MorphStage -> CharacterStage -> DriftProcessor -> VCO antialiasing -> VCO FM. Each layer depends on the one before it being solid.
- **Visual feedback first:** Display in Phase 1 means every subsequent phase's work is visible and debuggable immediately.
- **Character before drift:** Character defines the sound target; drift destabilizes it. Building drift first means tuning imperfections against an unknown target.
- **LFO before VCO:** The engine gets battle-tested at sub-audio rates where bugs are easier to diagnose. When VCO starts, the only new concerns are antialiasing, sync, and FM.
- **TZFM last:** It is the hardest feature with the most aliasing risk. Isolating it in the final phase prevents it from compromising base quality.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2 (Character Engine):** Analog reference waveform shapes need empirical tuning. The mathematical models (exponential ramp curvature, sigmoid edge softening) are starting points, not final values. Plan for iterative A/B testing against hardware recordings. Research should include capturing reference waveforms from actual synths if possible.
- **Phase 4 (VCO Antialiasing):** Morph-aware polyBLEP is the project's most technically demanding implementation. The discontinuity structure changes continuously with morph position (no discontinuities in sine/tri region, one in saw, two in square). Research the exact transition behavior at morph boundaries. Reference VCV Fundamental VCO-1 source code.
- **Phase 5 (Through-Zero FM):** Digital TZFM at high indices is a known unsolved problem. Research the practical limits and set user expectations. Investigate soft-limiting strategies and surgical oversampling for the FM path.

Phases with standard patterns (skip deep research):
- **Phase 1 (Foundation):** Well-documented VCV Rack patterns. POC validates the build system and basic waveform generation. Lock-free double buffer is a standard real-time audio pattern.
- **Phase 3 (Drift Engine):** Ornstein-Uhlenbeck processes and filtered noise are textbook DSP. Component spread via seeded PRNG is straightforward. Main work is perceptual tuning, not research.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | VCV Rack 2 SDK is stable and well-documented. POC validates build system. No external dependencies needed. Only uncertainty is exact latest SDK point release (2.5.x). |
| Features | MEDIUM-HIGH | Table stakes and differentiators well-identified. Competitive landscape analysis based on training data through May 2025 -- new analog-character competitors may have emerged. Feature prioritization is sound. |
| Architecture | HIGH | Three-layer separation (host/engine/display), lock-free display buffer, normalized internal representation are established patterns. PolyBLEP and morph crossfade architecture validated by VCV Fundamental and community modules. |
| Pitfalls | HIGH | DSP pitfalls (morph clicks, BLEP implementation, phase precision, TZFM aliasing) are mathematically grounded and well-documented in academic literature. VCV Rack-specific pitfalls (threading, serialization, voltage standards) confirmed by SDK documentation and community experience. |

**Overall confidence:** MEDIUM-HIGH

### Gaps to Address

- **Analog reference waveform accuracy:** The Minimoog saw, Roland square, etc. reference models are based on published circuit analysis, not direct measurements. Specific parameter values (ramp curvature percentage, edge softening time constants, THD levels) are educated starting points that must be tuned by ear against hardware recordings during Phase 2.
- **VCV Rack module landscape (current):** Competitive analysis is based on training data through May 2025. New analog-character modules may have launched. Verify the whitespace claim before marketing.
- **Morph-aware polyBLEP at transition boundaries:** The behavior when discontinuity count changes (one BLEP becoming two BLEPs as morph sweeps from saw to square) needs careful implementation and testing. No known reference implementation of this exact pattern.
- **PolyBLEP vs minBLEP decision:** STACK.md recommends minBLEP for quality; FEATURES.md and ARCHITECTURE.md use polyBLEP for simplicity and better morph compatibility. Recommendation: start with polyBLEP (simpler, better morph interaction), upgrade to minBLEP only if spectrum analysis shows insufficient suppression at high pitches.
- **Drift perceptual tuning:** Drift parameter values (cents of pitch wander, phase jitter amplitude, slew time constants) are literature-based starting points. Final values require systematic listening tests, ideally with multiple listeners.
- **SDK version verification:** Exact VCV Rack 2 SDK version (2.5.x assumed) and any API changes since May 2025 should be verified against live documentation before starting development.
- **Phase distortion feature:** Listed in PROJECT.md requirements but interaction with the morph engine is unclear -- may conflict with morph crossfade approach if it changes zero-crossing structure. Research during VCO implementation.

## Sources

### Primary (HIGH confidence)
- VCV Rack 2 SDK documentation (vcvrack.com) -- plugin architecture, threading model, DSP utilities
- Existing POC at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` -- validates build system, phase accumulation, waveform generation
- Stilson & Smith (CCRMA, 1996) -- Minimoog oscillator circuit analysis
- Valimaki et al. (2010) -- Virtual analog oscillator synthesis, polyBLEP/minBLEP techniques
- CEM3340 datasheet -- Roland SH-101/Juno-106 oscillator characteristics
- Eli Brandt, "Hard Sync Without Aliasing" -- antialiased hard sync techniques
- Ross Bencina, "Real-time audio programming 101" -- lock-free audio/GUI patterns

### Secondary (MEDIUM confidence)
- VCV Fundamental VCO source (GitHub) -- polyBLEP reference implementation
- NanoVG documentation (GitHub) -- display rendering API
- Prophet-5/Voyager service manuals -- triangle waveform characteristics
- musicdsp.org archives -- community DSP patterns and implementations
- Analog circuit analysis literature -- waveform bleed levels, component tolerance ranges

### Tertiary (LOW confidence)
- VCV Rack current module landscape -- based on training data, may have changed since May 2025
- Specific drift calibration values (cents, milliseconds) -- approximate, need perceptual validation
- FramebufferWidget vs TransparentWidget performance tradeoffs -- may vary with GPU/driver

---
*Research completed: 2026-02-25*
*Ready for roadmap: yes*
