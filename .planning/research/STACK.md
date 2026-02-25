# Technology Stack

**Project:** Forge Audio - Analog Series (VCV Rack LFO + VCO)
**Researched:** 2026-02-25
**Overall confidence:** MEDIUM-HIGH (VCV Rack 2 SDK is well-known and stable; cannot verify latest point releases against live docs)

## Recommended Stack

### Core Framework

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| VCV Rack 2 SDK | 2.5.x | Plugin host framework | The only option. VCV Rack modules are compiled C++ plugins against this SDK. The POC already uses it. | HIGH |
| C++17 | (compiler) | Implementation language | VCV Rack 2 requires C++17. The SDK's Makefile sets `-std=c++17`. Needed for `std::optional`, structured bindings, `if constexpr`, and `<cmath>` constexpr functions. | HIGH |
| GNU Make + plugin.mk | (SDK bundled) | Build system | VCV Rack's standard build. The SDK ships `plugin.mk` which handles cross-platform compilation, linking against Rack libraries, and dist packaging. Do not fight this with CMake. | HIGH |

### DSP - Waveform Generation

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| `rack::dsp::MinBlepGenerator` | SDK built-in | Antialiased discontinuity correction for VCO | The SDK ships a minBLEP generator in `<dsp/minblep.hpp>`. This is the standard approach used by VCV Fundamental VCO-1 and most serious VCV oscillator modules. PolyBLEP is simpler but minBLEP produces cleaner results at high frequencies. Use minBLEP for the VCO; the LFO does not need antialiasing (sub-audio rates). | HIGH |
| `rack::dsp::exp2_taylor5()` | SDK built-in | Fast exponential pitch conversion | Already used in the POC. Taylor-series approximation of 2^x, good enough for pitch CV-to-frequency conversion without calling `std::pow()` in the hot path. | HIGH |
| `rack::dsp::SchmittTrigger` | SDK built-in | Trigger detection for reset/sync | Already used in the POC. Standard VCV trigger detection with configurable hysteresis thresholds. | HIGH |
| Custom: Crossfade/morph engine | N/A (hand-rolled) | Waveform morphing | No library needed. Linear or equal-power crossfade between adjacent waveform shapes in the morph chain. This is straightforward math: `output = (1-t) * waveA + t * waveB` where t is the fractional morph position between the two nearest waveforms. | HIGH |
| Custom: Analog reference waveforms | N/A (wavetable + procedural) | Character knob targets | Store short single-cycle reference waveforms (2048 samples) captured/modeled from target synths (Minimoog saw, Roland square, etc.). Crossfade between the mathematical ideal and the reference waveform using the character knob. References can be procedurally generated at init time or loaded from static arrays. | MEDIUM |
| Custom: Drift engine | N/A (hand-rolled) | Analog imperfection modeling | Combine multiple low-frequency noise sources: slow random walk for pitch drift, faster jitter (sample-and-hold noise), per-instance component spread (set once on init), DC offset wander, first-order lowpass for HF rolloff, pitch slew (lag processor). No external library needed; this is basic DSP. | HIGH |

### DSP - Filters and Utilities

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| `rack::dsp::TRCFilter` | SDK built-in | First-order RC lowpass/highpass | Use for HF rolloff in the drift engine and for smoothing CV inputs. Simple, efficient, no external dependency. | HIGH |
| `rack::dsp::SlewLimiter` | SDK built-in | Pitch slew for drift | Provides asymmetric slew limiting. Use for the pitch slew component of the drift knob. | HIGH |
| `rack::dsp::ExponentialFilter` | SDK built-in | Parameter smoothing | Use for smoothing knob values to prevent zipper noise when morphing. One-pole exponential smoothing, very cheap. | HIGH |
| `rack::simd::float_4` | SDK built-in | SIMD operations | VCV Rack ships SSE-based 4-wide float SIMD wrappers. Use for processing 4 waveform shapes in parallel during morph calculations. Not essential for mono but gives headroom. | MEDIUM |

### Display and UI

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| NanoVG | SDK bundled (1.x) | Waveform display rendering | VCV Rack bundles NanoVG and exposes it through widget draw methods. All custom drawing (waveform trace, phase dot, background) uses NanoVG calls in `drawLayer()`. This is the only option for custom graphics in VCV Rack. | HIGH |
| `rack::widget::FramebufferWidget` | SDK built-in | Cached display rendering | Wrap the waveform display in a FramebufferWidget so it only re-renders when the waveform actually changes (dirty flag), not every frame. Critical for performance -- raw NanoVG redraws every frame are expensive. | HIGH |
| `rack::LightWidget` / `rack::ModuleLightWidget` | SDK built-in | Phase indicator dot | The phase-tracking dot can be implemented as a custom LightWidget positioned over the display, or drawn directly in the NanoVG path. Drawing directly in NanoVG is simpler and avoids widget hierarchy complexity. | HIGH |
| SVG panel via `rack::app::SvgPanel` | SDK built-in | Module faceplate | Standard VCV approach: SVG file in `res/` loaded as the panel background. POC already uses `createPanel()`. SVG must avoid filters, CSS, gradients with transforms, and text (convert to paths). nanosvg renderer is limited. | HIGH |

### Testing and Development

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| VCV Rack (application) | 2.5.x | Live testing environment | Run the plugin inside VCV Rack itself. Connect a scope module (e.g., Fundamental Scope) to visually verify waveforms. Connect a spectrum analyzer for aliasing checks. This IS the test harness. | HIGH |
| Fundamental Scope | (bundled) | Waveform verification | Free, ships with VCV Rack. Use to verify output waveform shapes match expectations. | HIGH |
| Bogaudio Analyzer-XL | Latest | Spectrum analysis | Free module, excellent FFT spectrum analyzer. Use to verify antialiasing quality by checking for aliased harmonics above Nyquist. Essential for VCO validation. | HIGH |
| Address Sanitizer (ASan) | Compiler built-in | Memory safety | Add `-fsanitize=address` to CXXFLAGS during development. Catches buffer overflows, use-after-free. Remove for release builds. | HIGH |

### Project Structure

| Component | Location | Purpose |
|-----------|----------|---------|
| Plugin entry | `src/plugin.cpp` / `src/plugin.hpp` | Model registration, plugin instance |
| LFO module | `src/AnalogLFO.cpp` | LFO Module + Widget classes |
| VCO module | `src/AnalogVCO.cpp` | VCO Module + Widget classes |
| Shared DSP | `src/dsp/MorphEngine.hpp` | Waveform morph + crossfade logic |
| Shared DSP | `src/dsp/CharacterEngine.hpp` | Analog reference waveform crossfade |
| Shared DSP | `src/dsp/DriftEngine.hpp` | All drift/imperfection components |
| Shared DSP | `src/dsp/AntiAlias.hpp` | MinBLEP wrapper for VCO |
| Display | `src/widgets/WaveformDisplay.hpp` | NanoVG waveform display widget |
| Reference data | `src/dsp/ReferenceWaveforms.hpp` | Static arrays or procedural generators for character targets |
| Panels | `res/AnalogLFO.svg`, `res/AnalogVCO.svg` | SVG faceplates |
| Metadata | `plugin.json` | Plugin and module metadata |
| Build | `Makefile` | Standard VCV plugin.mk include |

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Antialiasing | minBLEP (`MinBlepGenerator`) | PolyBLEP (polynomial bandlimited step) | PolyBLEP is simpler to implement (just a polynomial correction near discontinuities) but has worse high-frequency performance. At high VCO pitches (C6+), polyBLEP lets through audible aliasing that minBLEP suppresses. Since this is a quality-focused analog modeling project, use minBLEP. |
| Antialiasing | minBLEP | Oversampling (2x-4x) | Oversampling works but costs 2-4x CPU per sample. VCV Rack already runs at audio sample rates (44.1-192kHz); adding oversampling on top is expensive and unnecessary when minBLEP handles the problem at the discontinuity level. Reserve oversampling for nonlinear waveshaping if needed later. |
| Morph approach | Linear crossfade between adjacent shapes | Wavetable with interpolation | A wavetable approach (storing the full morph continuum as frames) wastes memory and adds complexity for something that is trivially computed as a weighted blend. Four base waveforms with three crossfade regions is simple, correct, and CPU-cheap. |
| Character approach | Single-cycle reference + crossfade | Full circuit modeling (SPICE-derived) | Full circuit modeling of a Minimoog oscillator is a research project, not a module feature. Single-cycle waveform references captured from or modeled after real synths (asymmetric saw, rounded square, etc.) crossed with the ideal waveform delivers 90% of the character for 1% of the complexity. |
| Drift noise | Multiple independent noise sources | Perlin noise | Perlin noise has a specific spectral character that does not match real analog drift. Real oscillator drift comes from multiple independent sources (thermal, component aging, power supply ripple) at different time scales. Combining separate random walks at different rates is more authentic and easier to tune. |
| Build system | GNU Make (plugin.mk) | CMake | VCV Rack's ecosystem assumes Make. The SDK ships plugin.mk. Every tutorial uses Make. Using CMake would require replicating all the SDK build logic and would confuse anyone familiar with VCV development. Do not fight the ecosystem. |
| Display caching | FramebufferWidget with dirty flag | Raw draw every frame | Drawing a full waveform path via NanoVG every frame (60fps) is expensive. FramebufferWidget renders to a texture and only redraws when marked dirty. Update the dirty flag when morph/character/drift values change beyond a threshold. The phase dot can be drawn in a lightweight overlay layer that does update every frame (it is just one circle). |
| SIMD | `rack::simd::float_4` for morph math | No SIMD | SIMD is not strictly necessary for mono operation, but computing 4 waveform values simultaneously (sine, tri, saw, square) and then selecting/blending is a natural fit for 4-wide SIMD. The SDK provides it free. Use it where it naturally fits but do not force it. |
| UI framework | NanoVG (SDK-bundled) | Dear ImGui / custom OpenGL | VCV Rack does not expose raw OpenGL to plugins. All rendering goes through NanoVG via the widget draw system. There is no alternative. |
| External DSP library | None (SDK built-ins + hand-rolled) | FAUST, Maximilian, STK | Adding external DSP libraries to a VCV Rack plugin is possible but creates build complexity, increases binary size, and the SDK already provides everything needed for this scope. The DSP here (oscillators, filters, noise) is simple enough to implement directly. FAUST would be overkill. |

## Key Implementation Notes

### MinBLEP Usage Pattern (VCO)

```cpp
// In the VCO's process() method:
// 1. Detect discontinuities (saw reset, square transition)
// 2. Insert minBLEP correction at the exact fractional sample position
// 3. Add the minBLEP buffer to the output

dsp::MinBlepGenerator<16, 16, float> minBlep; // 16-zero, 16-oversample

void process(const ProcessArgs& args) override {
    float deltaPhase = freq * args.sampleTime;
    phase += deltaPhase;

    // Saw: detect wrap-around
    if (phase >= 1.f) {
        phase -= 1.f;
        float fractional = phase / deltaPhase; // where in sample the reset occurred
        minBlep.insertDiscontinuity(fractional, -2.f); // saw drops by 2.0
    }

    float saw = 2.f * phase - 1.f;
    saw += minBlep.process(); // apply correction
}
```

### NanoVG Display Pattern

```cpp
struct WaveformDisplay : TransparentWidget {
    // Draw in layer 1 (above panel, below lights)
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1) return;
        if (!module) return;

        NVGcontext* vg = args.vg;

        // Background
        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(vg, nvgRGB(0x0d, 0x0d, 0x1a));
        nvgFill(vg);

        // Waveform trace
        nvgBeginPath(vg);
        nvgStrokeColor(vg, nvgRGB(0xe8, 0xa8, 0x38)); // forge amber
        nvgStrokeWidth(vg, 1.5f);

        for (int i = 0; i < DISPLAY_POINTS; i++) {
            float t = (float)i / (DISPLAY_POINTS - 1);
            float y = computeWaveformAtPhase(t); // evaluate morph+character at phase t
            float px = t * box.size.x;
            float py = (1.f - y) * 0.5f * box.size.y; // map [-1,1] to pixel Y
            if (i == 0) nvgMoveTo(vg, px, py);
            else nvgLineTo(vg, px, py);
        }
        nvgStroke(vg);

        // Phase dot (updates every frame)
        float dotPhase = module->phase;
        float dotY = computeWaveformAtPhase(dotPhase);
        nvgBeginPath(vg);
        nvgCircle(vg, dotPhase * box.size.x, (1.f - dotY) * 0.5f * box.size.y, 3.f);
        nvgFillColor(vg, nvgRGB(0xff, 0xff, 0xff));
        nvgFill(vg);
    }
};
```

### FramebufferWidget Caching Strategy

```cpp
// The waveform shape only changes when morph, character, or drift values change.
// The phase dot changes every frame.
// Solution: Split into two layers:
//   1. WaveformTraceWidget inside FramebufferWidget (cached, redraws on param change)
//   2. PhaseDotWidget as lightweight overlay (redraws every frame, just one circle)

struct CachedWaveformDisplay : FramebufferWidget {
    WaveformTraceWidget* trace;

    void step() override {
        // Check if params changed enough to warrant redraw
        if (module && paramsChanged()) {
            dirty = true; // triggers NanoVG re-render to texture
        }
        FramebufferWidget::step();
    }
};
```

## Do NOT Use

| Technology | Why Not |
|------------|---------|
| CMake | Fights the VCV Rack build ecosystem. plugin.mk handles everything. |
| External DSP libraries (FAUST, STK, Maximilian) | Adds build complexity for DSP that is straightforward to implement with SDK built-ins. |
| Oversampling for antialiasing | CPU cost is 2-4x. MinBLEP achieves the same result at the discontinuity level for negligible cost. |
| `std::thread` or async operations | VCV Rack's `process()` is single-threaded per module. All DSP must complete synchronously within one sample period. Threading creates race conditions with the audio thread. |
| Dynamic memory allocation in process() | No `new`, `malloc`, `std::vector::push_back` in the audio callback. Allocate everything at init time. |
| `std::sin()` in VCO hot path | Too slow for audio-rate per-sample calls. Use a polynomial approximation or lookup table for sine in the VCO. The SDK's waveform generation examples use Bhaskara or parabolic approximations. For LFO rates, `std::sin()` is fine. |
| Raw OpenGL calls | VCV Rack does not expose OpenGL to plugins. All rendering is through NanoVG. |
| Third-party UI frameworks | VCV Rack has its own widget system. ImGui, etc. cannot be injected. |
| C++20 or later | VCV Rack 2 SDK targets C++17. Using C++20 features will break builds on some platforms/compilers supported by the SDK. |
| Wavetable files (.wav, .wt) | For the character references, compile waveforms as static arrays in header files. Avoids file I/O, path resolution, missing file errors, and distribution issues. A 2048-sample float array is 8KB -- trivial. |

## Installation / Project Setup

```bash
# Clone or set up project structure
mkdir -p "Analog Series/src/dsp"
mkdir -p "Analog Series/src/widgets"
mkdir -p "Analog Series/res"

# Ensure Rack SDK is available (already at ../Rack-SDK from POC)
# The Makefile references RACK_DIR ?= ../Rack-SDK

# Build
cd "Analog Series"
make

# Install for testing (creates symlink in Rack plugins directory)
make install

# Clean build
make clean
```

### Makefile (minimal, based on POC)

```makefile
RACK_DIR ?= ../Rack-SDK

# Development: enable sanitizers
ifdef DEV
CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address
endif

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk
```

## Version Verification Status

| Component | Stated Version | Verified Against | Notes |
|-----------|---------------|------------------|-------|
| VCV Rack 2 SDK | 2.5.x | Training data only | Cannot verify latest point release. The 2.x API has been stable since 2022. MEDIUM confidence on exact version. |
| NanoVG | 1.x (bundled) | Training data only | Bundled with Rack, not independently versioned for plugins. API is stable. HIGH confidence on API. |
| MinBlepGenerator | SDK built-in | POC SDK headers (not accessible) | Well-established in Rack SDK since 2.0. HIGH confidence on existence and API. |
| C++17 | Compiler std | POC Makefile | Confirmed by standard VCV Rack build. HIGH confidence. |
| plugin.mk | SDK built-in | POC Makefile confirmed | Directly verified from POC. HIGH confidence. |

## Sources

- VCV Rack 2 Plugin Development Tutorial: https://vcvrack.com/manual/PluginDevelopmentTutorial (training data, not live-fetched)
- VCV Rack 2 SDK source: https://github.com/VCVRack/Rack (training data)
- VCV Fundamental VCO source (minBLEP reference): https://github.com/VCVRack/Fundamental/blob/v2/src/VCO.cpp (training data)
- Existing POC at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` (directly verified)
- NanoVG documentation: https://github.com/memononen/nanovg (training data)
- Eli Fieldsteel, "The Art of VA Filter Design" and Valimaki et al. on polyBLEP/minBLEP (academic, training data)

**Verification note:** WebSearch, WebFetch, and Bash tools were unavailable during this research session. All SDK-specific claims are based on training data knowledge of VCV Rack 2, cross-referenced with the existing POC code which confirms the build system, SDK structure, and core DSP utilities. Exact latest SDK version numbers should be verified before starting development.
