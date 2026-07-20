# Architecture Research

**Domain:** VCV Rack 2 audio-rate oscillator (VCO) added as a second module inside the existing `ForgeAudio-AnalogSeries` plugin
**Researched:** 2026-07-20
**Confidence:** HIGH on integration points and file wiring (read directly from source); MEDIUM on the morph-aware polyBLEP DSP design and alias-floor test tolerances (well-established technique, not yet implemented in this repo, no bit-frozen reference exists yet)

> Scope note: This is a *codebase-integration* architecture answer. Every integration point below was verified against the actual source (`plugin.cpp`, `plugin.hpp`, `Makefile`, `plugin.json`, `src/dsp/*.hpp`, `tests/*`). The DSP recommendations (polyBLEP/BLAMP wrapper, drift authority scaling) are standard oscillator practice adapted to this repo's bit-identity discipline.

---

## Standard Architecture

### System Overview — the "thin Rack shell delegates to a pure core" pattern, applied to a second module

```
┌──────────────────────────────────────────────────────────────────────────┐
│  RACK SHELL LAYER  (links libRack; C++11 strict-gated src/*.cpp)          │
│  ┌────────────────────────┐        ┌────────────────────────────────┐    │
│  │ src/AnalogLFO.cpp       │        │ src/AnalogVCO.cpp   ← NEW       │    │
│  │ (SHIPPED, DO NOT TOUCH) │        │ params/inputs/outputs enums     │    │
│  │ params/JSON/widget      │        │ widget · panel · display atomics│    │
│  │ delegates → LfoCore     │        │ JSON seeds · delegates → VcoCore│    │
│  └───────────┬─────────────┘        └───────────────┬────────────────┘    │
│              │  forge::Inputs (LFO POD)              │ forge::VcoInputs POD │
├──────────────┼───────────────────────────────────────┼────────────────────┤
│  PURE DSP CORE LAYER  (ZERO Rack includes; compiles under c++11 AND c++17) │
│  ┌───────────▼─────────────┐        ┌───────────────▼────────────────┐    │
│  │ src/dsp/LfoCore.hpp      │        │ src/dsp/VcoCore.hpp   ← NEW    │    │
│  │ (bit-frozen orchestrator)│        │ pitch→FM→phase→sync→BLEP→drift  │    │
│  └───────────┬─────────────┘        └────────┬──────────────┬─────────┘   │
│              │                                │              │             │
│              │                       ┌────────▼───────┐      │             │
│              │                       │ MorphBlep.hpp  │ ← NEW │             │
│              │                       │ (band-limits a │      │             │
│              │                       │  morph xfade)  │      │             │
│              │                       └────────┬───────┘      │             │
│  ┌───────────▼──────── SHARED LEAF HEADERS (reused AS-IS) ───▼─────────┐   │
│  │  Waveshape.hpp ·  MathConst.hpp ·  RackCompat.hpp                    │   │
│  │  (exp2_taylor5, clamp, Xoroshiro128Plus, SchmittTrigger, OnePole)    │   │
│  │  DriftEngine.hpp  ← MODIFIED ADDITIVELY (configurable authority)     │   │
│  └─────────────────────────────────────────────────────────────────────┘  │
├────────────────────────────────────────────────────────────────────────────┤
│  TEST LAYER  (Rack-free; -std=c++17; make test / make capture)             │
│  BlockDriver.hpp (LFO)   VcoBlockDriver.hpp ← NEW   golden/*.f32 fixtures   │
└────────────────────────────────────────────────────────────────────────────┘
```

The VCO does **not** invent a new architecture — it instantiates the exact same three-layer pattern the LFO already proves (`shell → POD Inputs → core.step() → output + telemetry`). The single genuinely-new subsystem is `MorphBlep` (band-limiting a *continuous* morph crossfade). Everything else is a second copy of a validated shape.

### Component Responsibilities

| Component | Responsibility | New / Reused / Modified |
|-----------|----------------|-------------------------|
| `src/AnalogVCO.cpp` | Rack `Module` + `ModuleWidget`: params/inputs/outputs enums, panel wiring, display atomics, drift-seed JSON persistence, per-sample marshalling of Rack I/O into `VcoInputs` and back | **NEW** |
| `src/dsp/VcoCore.hpp` | Pure orchestrator: `VcoInputs → step() → float volts + telemetry`. Owns pitch math, exp FM, phase accumulator, hard-sync reset, drift application, calls `MorphBlep` | **NEW** |
| `src/dsp/MorphBlep.hpp` | Band-limits the naive morphed waveform: polyBLEP at value discontinuities (saw/square/pulse jumps + sync reset), BLAMP at slope discontinuities (triangle corners), morph- and character-weighted | **NEW** |
| `src/dsp/Waveshape.hpp` | Naive per-sample shape + morph crossfade + bleed. Called by `MorphBlep`/`VcoCore` for the un-band-limited value and for measuring discontinuity heights | **REUSED as-is** (bit-frozen for LFO golden) |
| `src/dsp/RackCompat.hpp` | `exp2_taylor5` (V/oct + FM), `clamp`, `Xoroshiro128Plus` (drift RNG), `SchmittTrigger` (sync edge detect), `OnePole` | **REUSED as-is** |
| `src/dsp/MathConst.hpp` | `forge::kPi` | **REUSED as-is** |
| `src/dsp/DriftEngine.hpp` | 4-layer OU + jitter + DC drift. Transfers to audio rate unchanged in *dynamics*; **authority constants** must become configurable | **MODIFIED additively** |
| `src/dsp/DisplayFill.hpp`, `PatchParse.hpp`, `Anim.hpp` | Single-cycle preview fill (already takes explicit `bleedLfo`), safe hex-seed parse, animation easing | **REUSED** by the VCO shell (candidates) |
| `tests/VcoBlockDriver.hpp` | Headless driver holding `forge::VcoCore` (parallel to `BlockDriver.hpp`) | **NEW** |

---

## Recommended Project Structure

```
src/
├── AnalogLFO.cpp          # SHIPPED shell — untouched
├── AnalogVCO.cpp          # NEW shell (auto-compiled by SOURCES += $(wildcard src/*.cpp))
├── plugin.cpp             # MODIFIED: + p->addModel(modelAnalogVCO);
├── plugin.hpp             # MODIFIED: + extern Model* modelAnalogVCO;
└── dsp/
    ├── Waveshape.hpp      # reused as-is  (bit-frozen)
    ├── DriftEngine.hpp    # MODIFIED additively (authority fields, defaults = LFO literals)
    ├── RackCompat.hpp     # reused as-is
    ├── MathConst.hpp      # reused as-is
    ├── LfoCore.hpp        # untouched
    ├── VcoCore.hpp        # NEW orchestrator
    └── MorphBlep.hpp      # NEW band-limiting wrapper
res/
├── AnalogLFO.svg         # untouched
└── AnalogVCO.svg         # NEW panel (Forge Noir; Rack derives HP from viewBox)
tests/
├── BlockDriver.hpp       # untouched (LFO)
├── VcoBlockDriver.hpp    # NEW
├── test_vco_invariants.cpp   # NEW
├── test_vco_golden.cpp       # NEW
└── golden/
    ├── freerun_*.f32     # LFO fixtures — MUST stay byte-identical
    ├── vco_*_driftoff.f32    # NEW portable VCO fixtures
    └── vco_seeds.txt         # NEW
tools/
└── capture_vco_golden.cpp    # NEW (mirror of capture_golden.cpp for the VCO core)
plugin.json               # MODIFIED: second entry in modules[]
Makefile                  # MODIFIED minimally: add capture-vco target (source wildcards already cover the rest)
```

### Structure Rationale

- **No duplication of the analog engine.** `Waveshape`/`DriftEngine`/`RackCompat`/`MathConst` are already Rack-free leaf headers designed to be shared. The VCO includes them directly — this is the whole point of the v1.4 extraction and the reason the milestone can be "lean."
- **`MorphBlep` is a *wrapper*, not new methods inside `Waveshape`.** Keeping the band-limiting in its own header structurally protects the LFO golden (see Pattern 2). `Waveshape.hpp` stays a file nobody edits.
- **The shell wildcard and strict/CI gates need zero new plumbing.** `SOURCES += $(wildcard src/*.cpp)` auto-picks `AnalogVCO.cpp`; `TEST_SOURCES/TEST_HEADERS` wildcards auto-pick the new tests and dsp headers; the `strict` C++11 gate globs `src/*.cpp`. The only Makefile edit is an optional `capture-vco` convenience target.

---

## Architectural Patterns

### Pattern 1: POD `VcoInputs` boundary — the core never sees a Rack index

**What:** A flat struct marshals all Rack I/O into the core (mirrors `forge::Inputs` for the LFO). The core reads only plain values; `sampleTime` is *injected*, never read from a global.

**When to use:** Always — it is what makes the core Rack-free, unit-testable, and golden-capturable.

**Trade-offs:** One extra copy per sample (negligible). Buys full headless testability and bit-reproducibility.

**Recommended fields:**
```cpp
namespace forge {
struct VcoInputs {
    // --- pitch (all in the volt/octave domain; summed before one exp2) ---
    float pitchCV     = 0.f;   // V/OCT input volts (0 if unpatched)
    float coarse      = 0.f;   // COARSE knob mapped to octaves/semitones (volts)
    float fine        = 0.f;   // FINE knob mapped to cents (volts)
    // --- exponential FM (added into the pitch volts BEFORE exp2) ---
    float fmVolts     = 0.f;
    float fmAtten     = 0.f;
    bool  fmConnected = false;
    // --- analog engine (post-CV, post-clamp [0,1]; shell folds characterSpread in) ---
    float morph       = 0.f;
    float character   = 0.f;
    float drift       = 0.f;
    // --- hard sync ---
    float syncVoltage = 0.f;
    bool  syncConnected = false;
    // --- injected timing (harness/shell owns this) ---
    float sampleTime  = 1.f / 44100.f;
    float sampleRate  = 44100.f;   // handy for a Nyquist freq clamp / alias tests
};
}
```
Note the deliberate divergence from the LFO POD: **no `rate`/`ratioScaled`/clock/reset/swing/phaseOffset** (those are LFO-domain), and pitch is volt-domain so a single `exp2_taylor5` converts it. Keep `morph/character/drift` identically named so the shared `Waveshape`/`DriftEngine` wiring is copy-paste.

### Pattern 2: Band-limit by *wrapping* the frozen shape, never by editing it

**What:** `MorphBlep` calls `Waveshape::morphedWave()` for the naive value, then subtracts polyBLEP/BLAMP residuals at the known discontinuity phases. It measures the *actual rendered* jump height by sampling the shape just-before/just-after each discontinuity.

**When to use:** For every audio-rate render. At LFO rates the residuals are ~0 (deltaPhase tiny) so the technique is harmless — but the LFO does not use this path at all.

**Trade-offs:** A wrapper re-evaluates a couple of shape samples near each discontinuity (a few extra `sin`/`tanh` per sample). In exchange, `Waveshape.hpp` is never touched, so the LFO golden cannot regress. **Strongly preferred over adding `bandlimitedX()` methods inside `Waveshape.hpp`:** adding methods is *technically* golden-safe (existing methods unchanged) but it erodes the "this file is bit-frozen, do not edit" contract and invites an accidental one-character change to a shared method that silently breaks `freerun_*.f32`.

**Why numeric height-detection (not analytic):** the `character` knob *softens* edges (tanh edges, cosine capacitor reset, rounded triangle peaks). That shrinks the effective discontinuity. Measuring `shape(disc⁺) - shape(disc⁻)` on the already-characterized wave auto-scales the BLEP correction with character — high character needs less correction, and the wrapper gets that for free. An analytic jump table would have to re-derive every character deformation.

**Sketch:**
```cpp
// Known discontinuity phases for the morphed wave:
//   phase wrap   (0.0): saw/square/pulse value jump   -> polyBLEP
//   duty edge    (d):   square/pulse falling edge      -> polyBLEP
//   tri corners  (0.0, valley): slope break            -> BLAMP (polyBLAMP)
// residual is scaled by the measured jump/slope magnitude, weighted by how much
// of the current morph segment is saw/square/pulse (value) vs triangle (slope).
float y = wave.morphedWave(phase, morph, character, bleedLfo);
y -= blepAt(phase, dt, wrapJumpHeight);      // if within dt of a value discontinuity
y += blampAt(phase, dt, cornerSlopeDelta);   // if within dt of a slope discontinuity
```

### Pattern 3: Additive drift-authority — audio-rate reuse without touching the LFO golden

**What:** `DriftEngine`'s OU *dynamics* are time-parameterized (`theta`,`sigma` in per-second units, `dt`+`sqrt(dt)` injected), so they are **sample-rate-independent by construction** — smaller audio-rate `dt` makes the Euler–Maruyama step *more* accurate, not broken. What does **not** transfer is the hardcoded *authority*: `maxDrift = 0.075` (free) means 7.5 % deltaPhase wobble. At LFO rates that is musical; at, say, 440 Hz it is ~1.3 semitones of drift — a badly-detuned VCO. Per-sample jitter (0.3 %) and DC wander are likewise LFO-tuned and, at audio rate, jitter becomes audible broadband noise.

**When to use:** The VCO needs a much smaller authority (a few cents → ~0.3–1 % max deltaPhase; jitter near-zero; DC near-zero).

**How to keep it additive (golden-safe):** replace the three hardcoded literals in `DriftEngine::step()` with member fields whose **defaults equal today's literals**:
```cpp
struct DriftEngine {
    float maxDriftFree = 0.075f, maxDriftClocked = 0.02f;   // defaults == current literals
    float jitterAuthorityFree = 0.075f, jitterAuthorityClocked = 0.02f;
    float dcAuthorityFree = 0.075f, dcAuthorityClocked = 0.02f;
    // ... step() reads these members instead of the literals ...
};
```
IEEE-754 makes this bit-identical for the LFO: the field holds the *same* `float` value `0.075f`, feeding the *same* multiply — no reordering, no `×1.0` trick needed. The VCO simply sets smaller values after construction. **Non-negotiable verification:** after this change, `make test` must replay `freerun_*.f32` byte-for-byte. That golden replay is the guardrail that proves the LFO is untouched, so schedule this change *early and in isolation* (see Build Order Phase B), not buried inside the polyBLEP work.

> Alternative considered: a defaulted trailing `authorityScale = 1.f` argument to `step()`. Also safe (`x * 1.0f == x` exactly) and smaller diff, but it scales all three authorities by one number; the member-field form lets the VCO tune jitter → ~0 independently of slow drift, which matters at audio rate. Recommend the member-field form.

### `VcoCore::step()` per-sample sequence (the ordering contract)

```
 1. pitch   = coarse + fine + pitchCV                       (volts, volt/oct domain)
 2. if fmConnected: pitch += fmVolts * fmAtten * fmDepth     (exp FM = add in volt domain)
 3. freq    = FREQ_C4 * exp2_taylor5(pitch)                  (C4 = 261.6256 Hz)
             clamp freq to (0, ~sampleRate*0.5)              (Nyquist guard)
 4. deltaPhase = (double)freq * (double)sampleTime           (double accum, matches LFO)
 5. drift:  DriftEngine.step(...VCO authority...) -> deltaPhase *= deltaPhaseMul
             (drift-low skip < 0.001 preserved; bleedLfo still fed to morphedWave)
 6. hard sync: SchmittTrigger on syncVoltage. On rising edge, compute the
             sub-sample reset fraction, force phase := 0 (or the reset target),
             and STAGE a sync-BLEP correction for step 8.
 7. phase  += deltaPhase; wrap [0,1)                          (do this AFTER sync reset)
 8. sample  = MorphBlep.render(phase, deltaPhase, morph, character, bleedLfo, syncResidual)
             = naive morphedWave  -  polyBLEP(wrap,duty,sync)  +  BLAMP(tri corners)
 9. out     = 5.f * sample   (+ optional tiny dcOffsetV; usually ~0 for a VCO)
10. telemetry: displayPhase, freq/pitch, sync-fired flag  -> shell display atomics
```
Rationale for the order: exponential FM must land in the **volt domain before** the single `exp2` (that is exactly how the LFO's FM path already works and why it reuses `exp2_taylor5`); drift multiplies deltaPhase (same as LFO); sync reset happens **before** the phase accumulate so the wrap logic and the sync-BLEP fraction agree; band-limiting is the last thing before scaling to ±5 V.

---

## Data Flow

### Per-sample audio flow (VCO)

```
[V/OCT · COARSE · FINE · FM · SYNC · MORPH/CHAR/DRIFT knobs+CV]  (Rack params/inputs[])
        ↓  shell marshals + CV-mixes + clamps
[forge::VcoInputs POD]  (sampleTime injected)
        ↓
VcoCore::step():  pitch → exp2_taylor5 → freq → deltaPhase → drift → sync → phase
        ↓                                   ↓ calls
   Waveshape::morphedWave  ←────────  MorphBlep (BLEP/BLAMP residuals)
        ↓
[±5 V float]  → outputs[VCO_OUTPUT]        [Telemetry]  → shell display atomics (lock-free)
```

### Golden-capture flow (test)

```
tools/capture_vco_golden.cpp  → constructs forge::VcoCore directly,
   seeds ONLY the drift RNG, leaves *Spread at 0.f  (portability!)  → vco_<rate>_driftoff.f32
make test → test_vco_golden.cpp replays the same seeds/scenario → byte-compares
```
This mirrors the LFO's split exactly: the portable golden half exercises only `Xoroshiro` uniform + libm `sin/cos`/`tanh` (deterministic across libc++/libstdc++/MinGW); the non-portable `std::normal_distribution` drift/spread path is deliberately **not** captured (it stays behind on-host invariant assertions, not byte-goldens).

---

## Scaling Considerations

For a Rack module "scale" means **sample rate and CPU per voice**, not users.

| Axis | Consideration |
|------|---------------|
| 44.1 / 48 / 96 kHz | Every invariant parametrizes over all three (as the LFO tests already do). BLEP residual width is `deltaPhase` (= freq/sr), so it self-scales; no per-rate constants. |
| High notes near Nyquist | This is the whole reason for `MorphBlep`. Without it, a 2 kHz saw at 44.1 k folds audible images. Clamp `freq` below Nyquist and rely on BLEP/BLAMP for the harmonics below it. v2.0 is **1×** — oversampling (2×/4×) is deliberately deferred to v2.1. |
| CPU per sample | Naive morph already computes 4–5 `compute*` shapes/sample; `MorphBlep` adds a few boundary re-evaluations. Monophonic by design (polyphony is Out of Scope), so this is fine. Profile only if `perf` shows the shape stack dominating. |

### Scaling priorities

1. **First thing that "breaks": aliasing on high notes.** Fixed by `MorphBlep` (Phase C). Everything else is comfortably within budget at 1× mono.
2. **Second: CPU if morph re-evaluates too many shapes.** Only optimize (e.g. skip inactive segments) *after* the alias-floor test passes — correctness before speed.

---

## Anti-Patterns

### Anti-Pattern 1: Adding `bandlimited*()` methods into `Waveshape.hpp`

**What people do:** Extend the shared shape header with polyBLEP variants "since it's the natural home."
**Why it's wrong:** `Waveshape.hpp` is bit-frozen — `freerun_*.f32` and the LFO's shipped sound depend on it byte-for-byte. Editing the file (even adding methods) removes the "never touch" contract and is one careless refactor away from regressing the shipped LFO.
**Do this instead:** Put band-limiting in `MorphBlep.hpp`, which *calls* `Waveshape`. The shared file stays read-only.

### Anti-Pattern 2: Reusing the LFO's drift authority verbatim at audio rate

**What people do:** Instantiate `DriftEngine`, call `step()`, ship — because "it already works."
**Why it's wrong:** 7.5 % deltaPhase drift = ~1.3 semitones detune; per-sample jitter becomes audible noise. The VCO sounds broken/out-of-tune.
**Do this instead:** Configurable authority members with LFO defaults (Pattern 3); set VCO authority to a few cents / near-zero jitter; re-run the LFO golden to prove no regression.

### Anti-Pattern 3: A per-module `random_device` inside the core

**What people do:** Seed the drift RNG from OS entropy inside `VcoCore`.
**Why it's wrong:** Kills determinism and golden reproducibility; the existing core forbids Rack/OS entropy inside `src/dsp/`.
**Do this instead:** The shell reads OS entropy and forwards explicit seeds via `core.seed()/setSpreadSeed()` (exactly as `AnalogLFO.cpp` does), persisting them through `dataToJson/dataFromJson` with `PatchParse`.

### Anti-Pattern 4: C++17-isms / in-class ODR-used `static constexpr` in the new headers

**What people do:** Add a `static constexpr float BLEP_TABLE[]` inside a struct, or use `if constexpr`/structured bindings.
**Why it's wrong:** The plugin + `make strict` build at `-std=c++11 -pedantic-errors`; recent commits (`8615945`, CI toolchain gate) fixed exactly this class of ODR/`c++11` failures that local clang masked. `MorphBlep`/`VcoCore` are included by `AnalogVCO.cpp` (c++11) *and* the tests (c++17), so they must compile clean at c++11.
**Do this instead:** Free `inline` functions / plain constants; if a `static constexpr` array is ODR-used, define it out-of-line, per the established fix.

---

## Integration Points

### Registration (mechanical, non-DSP) — the full checklist

| File | Change | Notes |
|------|--------|-------|
| `src/plugin.hpp` | `extern Model* modelAnalogVCO;` | next to `modelAnalogLFO` |
| `src/plugin.cpp` | `p->addModel(modelAnalogVCO);` in `init()` | one line |
| `src/AnalogVCO.cpp` | `Model* modelAnalogVCO = createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO");` | auto-compiled by `SOURCES += $(wildcard src/*.cpp)` — **no Makefile edit** |
| `plugin.json` | second entry in `modules[]` (`slug:"ForgeAnalogVCO"`, name, tags, `manualUrl`) | slug is permanent — pick carefully |
| `res/AnalogVCO.svg` | new Forge Noir panel | Rack derives HP from `viewBox`; no `width` in `plugin.json` (D-03 convention) |

The plugin `slug` stays `ForgeAudio-AnalogSeries`, one binary, one VCV Library entry (issue #929 remains canonical) — the VCO is an *added module*, not a new plugin.

### Build / test / CI integration

| Boundary | Status |
|----------|--------|
| `SOURCES += $(wildcard src/*.cpp)` | auto-picks `AnalogVCO.cpp` — no change |
| `TEST_SOURCES/TEST_HEADERS` wildcards | auto-pick `test_vco_*.cpp`, `VcoBlockDriver.hpp`, `VcoCore.hpp`, `MorphBlep.hpp` — no change |
| `make strict` (`$(wildcard src/*.cpp)`) | auto-covers `AnalogVCO.cpp` — no change; new dsp headers must pass c++11 |
| `make capture` | add a `capture-vco` target (or extend the existing one) → `tools/capture_vco_golden.cpp` → `tests/golden/vco_*_driftoff.f32` |
| GitHub Actions CI | already runs `make test` + strict + MinGW link; new tests/goldens ride along automatically |

### Internal boundary: `MorphBlep` ↔ `Waveshape`

Direct header include (leaf → leaf). `MorphBlep` depends on `Waveshape` (naive value + boundary re-samples) and `MathConst`; it must **not** depend on `VcoCore` (keep it a reusable, independently-testable leaf, so a future v2.1 oversampler can wrap it too).

---

## Test / Golden Strategy

**Non-negotiable invariant across the whole milestone:** the LFO goldens `tests/golden/freerun_*.f32` stay **byte-identical**. Every phase that touches a shared header (only DriftEngine, Phase B) ends with a `make test` golden replay as its gate.

New harness + fixtures (parallel to the LFO's, never merged into them):

| Artifact | Purpose |
|----------|---------|
| `tests/VcoBlockDriver.hpp` | Holds `forge::VcoCore`; drives N samples, injects `sampleTime=1/sr`; non-degenerate default seeds (avoid the `(0,0)` Xoroshiro fixed point, same landmine as the LFO driver) |
| `tools/capture_vco_golden.cpp` | Constructs `VcoCore` directly, seeds drift RNG only, spread left at 0 → `vco_<rate>_driftoff.f32` |
| `tests/golden/vco_seeds.txt` | Records the exact drift seeds + scenario for reproducible replay |
| `tests/test_vco_golden.cpp` | Byte-replay of the portable drift-off fixtures at 44.1/48/96 k |
| `tests/test_vco_invariants.cpp` | Behavioral invariants below, parametrized over the three rates |

VCO invariants (the LFO's ±5 V/period/continuity/determinism set, re-specified for a VCO):

| # | Invariant | Suggested bound (tune during Phase A/C) |
|---|-----------|------------------------------------------|
| 1 | **V/oct tracking** | drive `pitchCV ∈ {-2..+4} V`, measure period over ≥ many cycles; `|f_measured / (FREQ_C4·2^pitch) − 1|` within ~0.05 % (a few cents). This is the headline correctness test. |
| 2 | **Output bounds** | `[-5.0, +5.0] V` drift-off (±1e-4 FP slop); a small margin drift-on (DC/BLEP overshoot budget — measure, don't guess). |
| 3 | **Alias floor / spectral** | render a high note (e.g. 2–4 kHz at 44.1 k), DFT the block, assert energy at the mirror/image frequencies is below a floor (target ≈ −60 dB rel. fundamental, refine empirically). **Requires a small DFT/Goertzel helper** in the test — this is the novel, hardest assertion. |
| 4 | **Sync continuity** | across a hard-sync reset, no full-scale step artifact: `|out[n]−out[n−1]|` bounded (a BLEP reset is *not* zero-derivative like the LFO's cosine crossfade, so the bound is looser — quantify against the un-BLEP'd baseline). |
| 5 | **Fixed-seed determinism** | same seed → bit-identical block (drift on); different seed diverges. Identical mechanism to the LFO. |

Confidence: invariants 1/2/5 are HIGH (direct analogues of shipped LFO tests). Invariant 3 (alias floor) and the exact tolerance on 4 are MEDIUM — standard practice, but the numbers should be pinned empirically once the naive and band-limited versions exist to compare.

---

## Suggested Build Order (for the roadmapper)

Sequenced so the hard, risky work (polyBLEP, sync) is isolated *after* the "don't break the LFO" guardrail is in place, and each phase is independently testable/shippable-green.

| Phase | Deliverable | Depends on | De-risks |
|-------|-------------|-----------|----------|
| **A. VcoCore skeleton** | `VcoInputs` POD + V/oct pitch via `exp2_taylor5` + naive `morphedWave` at audio rate + `VcoBlockDriver` + invariants 1 (tracking) & 5 (determinism). *Aliased on purpose* (no BLEP yet); alias test not yet asserted. | Waveshape/RackCompat/DriftEngine reused as-is | Proves pitch accuracy + the boundary + headless harness before any hard DSP |
| **B. Drift authority (additive)** | Configurable authority members on `DriftEngine` (defaults = LFO literals); VcoCore sets small VCO authority; VCO drift invariant. **Gate: `make test` replays `freerun_*.f32` byte-identical.** | A | Locks in the "LFO golden preserved" guarantee *early*, before it can be entangled with BLEP changes |
| **C. Morph-aware polyBLEP** | `MorphBlep.hpp`: BLEP for saw first, then square/pulse, then BLAMP for triangle. Add invariant 3 (alias floor) + DFT helper. Iterate here. | A (and Waveshape) | The single genuinely-hard subsystem, fully isolated in its own header + test |
| **D. Hard sync + sync-BLEP** | `SchmittTrigger` sync edge, sub-sample reset fraction, sync-BLEP residual reusing Phase-C machinery. Invariant 4 (sync continuity). | C | Builds on proven BLEP residual code |
| **E. Exponential FM** | Fold `fmVolts·fmAtten·depth` into the pitch volt domain before `exp2`. FM depth/tracking test. | A | Small; reuses the exact FM pattern the LFO already ships |
| **F. Shell + panel + register** | `AnalogVCO.cpp` (enums, widget, `res/AnalogVCO.svg`, display atomics reusing `DisplayFill`/`Anim`, drift-seed JSON via `PatchParse`), plugin.hpp/plugin.cpp/plugin.json wiring. In-Rack UAT. | A–E | First point Rack is involved; core already proven headless |
| **G. Golden + CI + strict** | `capture_vco_golden.cpp`, `vco_*.f32`, `test_vco_golden.cpp`, `make capture-vco`, confirm `make strict` (c++11) passes for the new headers/shell + MinGW CI link. | A–F | Locks portability; the c++11/ODR gate catches the exact class of bug that sank v2.0.0 |

Mapping to the question's proposed order (a…f): this refines it by (1) inserting **Phase B** (drift authority + LFO-golden guardrail) between skeleton and polyBLEP, and (2) treating golden/CI/strict as a continuous discipline (each phase stays green) with a final consolidation in **G**, rather than a single end step. The rest matches: A=skeleton, C=polyBLEP, D=sync, E=FM, F=shell.

---

## Sources

- `src/dsp/LfoCore.hpp`, `Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp` — the pattern, POD boundary, bit-identity landmines, and reusable primitives (read directly, HIGH confidence)
- `src/plugin.cpp`, `src/plugin.hpp`, `plugin.json`, `Makefile` — registration + build/test/strict/CI wiring (read directly, HIGH confidence)
- `tests/BlockDriver.hpp`, `tests/test_invariants.cpp`, `tools/capture_golden.cpp` — headless harness + golden split model to mirror (read directly, HIGH confidence)
- `.planning/PROJECT.md` — milestone scope (lean core, deferrals), constraints, and the v2.0.0-rejection/strict-c++11 lesson (HIGH confidence)
- polyBLEP / polyBLAMP band-limited oscillator technique — standard DSP practice for antialiasing saw/square (value discontinuity → BLEP) and triangle (slope discontinuity → BLAMP); adaptation to a continuous morph crossfade with character-softened edges is this project's specific design (MEDIUM confidence — established method, no bit-frozen reference exists in-repo yet, tolerances to be pinned empirically)

---
*Architecture research for: VCV Rack 2 morphing VCO added to an existing multi-module plugin*
*Researched: 2026-07-20*
