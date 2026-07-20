# Feature Research

**Domain:** VCV Rack 2 analog-modeled *morphing* VCO (audio-rate oscillator), second module in the `ForgeAudio-AnalogSeries` plugin
**Researched:** 2026-07-20
**Confidence:** HIGH (VCV voltage/behavior conventions are formally documented; reference modules verified against VCV Library + vendor docs; engine reuse verified by reading `Waveshape.hpp` / `DriftEngine.hpp`)

> Scope note: This supersedes the prior FEATURES.md at this path (v1.4 *documentation/manual* research, 2026-06-14). This file is product-feature research for the v2.0 VCO module.

## Reference set analyzed

| Module | Class | Outputs | FM | Sync | Poly | Relevance |
|--------|-------|---------|----|----|------|-----------|
| Fundamental VCO-1/VCO-2 | bread-and-butter analog | simultaneous sin/tri/saw/sqr | expo + linear | hard/soft | yes | baseline table-stakes reference |
| Befaco EvenVCO | analog | tri/ramp/pwm/sine + **Even** | linear | hard | **mono** | precedent: a respected VCO can be mono |
| Bogaudio VCO/XCO | analog | simultaneous sqr/saw/tri/sine | expo + linear **TZFM** | hard | yes | feature-maximal reference (what we deliberately don't do) |
| Surge XT VCO (Classic/Modern/…) | multi-model | **single** out | per-model | per-model | yes | precedent: model/morph oscillator with one output |
| Audible Instruments Macro Osc (Mutable Braids/Plaits) | macro/morph | **single** out (MODEL morphs timbre) | expo (+TZ some) | yes | yes | closest paradigm match — morph → single output is idiomatic |
| Instruō Cš-L | complex analog | multiple (wavefolder per osc) | linear | yes | — | morphing/symmetry-bias reference, but multi-out complex voice |

**Key framing:** The Forge VCO is a **morph/macro oscillator**, not a bread-and-butter multi-out analog VCO. Its correct peer group is Plaits/Braids/Surge (single output, timbre is a knob), *not* Fundamental/Bogaudio (many simultaneous shape outs). This reframes the "single output" question from a limitation into a genre convention (see Anti-Features).

## Feature Landscape

### Table Stakes (Users Expect These)

Missing any of these makes it not feel like a real VCV VCO.

| Feature | Why Expected | Complexity | Engine dependency / notes |
|---------|--------------|------------|---------------------------|
| **1V/oct pitch input, accurate tracking** | The defining contract of a VCV VCO. `f = f₀·2^V`, baseline C4 = 261.626 Hz at 0V (VCV Voltage Standards). Users expect clean tracking over **at least 7–10 octaves**. | MEDIUM | Reuses the existing `exp2_taylor5` pitch path (already in the LFO's FM). Base tracking must be accurate *independent* of drift; drift rides on top. |
| **Coarse + fine tune** | Every VCO has it — coarse for octave/interval placement, fine for beating/unison detune. | LOW | Recommend COARSE ≈ ±5 octaves continuous (optional right-click semitone/octave snap — meaningful for a VCO, unlike the LFO where it was out-of-scope), FINE ≈ ±1 semitone (±100 cents). Pure pitch summation into the exp2 path. |
| **±5 V audio output (10 Vpp)** | VCV standard: oscillators output ±5 V before bandlimiting; saturate to avoid clipping. | LOW | Already the LFO output convention. Reuse the ±5 V morphed output; apply soft saturation so drift DC-offset + bleed can't push past rails. |
| **Exponential audio-rate FM + attenuverter** | Expo FM is standard; a bare FM jack with no depth control is considered incomplete — the attenuverter is table stakes. | MEDIUM | Reuses the LFO expo FM path at audio rate. **New vs LFO:** a dedicated FM attenuverter (LFO FM had fixed/scaled authority). This is the one genuinely new panel control the VCO must add. |
| **Hard sync input** | Expected on any subtractive-synth VCO; the primary way users get classic sync-sweep timbres. | MEDIUM | Not in the LFO. Reset phase accumulator on rising edge. Must interact with the morph-aware polyBLEP — the sync discontinuity injected mid-cycle needs its own BLEP correction, else it aliases. Coupled to the anti-aliasing work. |
| **Band-limited output (no obvious aliasing)** | A VCO that aliases audibly is considered broken; every reference module band-limits. | HIGH | **The hard problem.** morph-aware polyBLEP over a *continuous* crossfade + character-deformed edges + sync discontinuities. The milestone's central risk (already flagged in PROJECT). Owned by STACK/ARCHITECTURE; listed here as the invisible table stake. |
| **Wide, musical pitch range** | Users expect roughly C-1…C8/C9 reachable (sub-bass to high leads). | LOW | Falls out of accurate 1V/oct + coarse range; just clamp to a sane max (avoid Nyquist blow-ups at extreme +V/oct). |
| **Real-time timbre display** | Carried expectation from the LFO; this plugin's users expect the CRT. | MEDIUM | See Display section — static single-cycle morph preview, not a live scope. |

**Polyphony — the honest gap.** VCV *recommends* up to 16 channels keyed off the V/oct input, and the modern "default" VCOs (Fundamental, Bogaudio) are polyphonic. The plugin-wide decision explicitly **excludes polyphony** (16× CPU; complicates per-voice drift and the single-cycle display). Assessment:

- **Defensible, not free.** Befaco EvenVCO (well-regarded) is mono, and the whole *character/boutique* oscillator niche is routinely mono. A morph/drift oscillator is a "voice," not a "poly workhorse."
- **The drift identity is inherently per-voice.** Doing poly *correctly* means 16 independent `DriftEngine` instances (16× RNG state + 16× OU stepping) and 16 component-spread seeds — exactly the cost the decision cites. Sharing one drift engine across voices would sound wrong and undercut the identity.
- **Duplication is idiomatic here and sonically desirable:** each instance already has its own seed → own component spread + drift, so stacking 2–3 instances gives natural analog unison/detune for free. A genuine plus, not just a workaround.
- **Verdict:** Mono is acceptable for v2.0 *if documented plainly* and positioned as "one alive voice." It remains the single most legitimate feature request users will raise, and the strongest **v2.1+** candidate — but it is a *plugin-wide* decision to revisit, not a VCO-local one.

### Differentiators (Competitive Advantage)

Where the module earns its place — the morph/character/drift identity at audio rate. All three are **inherited directly** from the shipped engine (`Waveshape.hpp` + `DriftEngine.hpp`), so marginal build cost is low; the real cost is *calibration* for audio rate.

| Feature | Value Proposition | Complexity | Engine dependency / notes |
|---------|-------------------|------------|---------------------------|
| **Continuous MORPH as the sole timbre axis** | One knob sweeps sine→tri→saw→square→narrow-pulse with neighbor bleed. Positions the module with Plaits/Surge (timbre is a CV-able, automatable knob), not with 4-jack analog VCOs. | LOW | Verbatim `Waveshape::morphedWave()`. Only new work: the crossfade must be band-limited (polyBLEP) at audio rate — see table-stakes anti-aliasing. |
| **CHARACTER as an oscillator-timbre coloration knob** | At audio rate the per-shape analog modeling *is* harmonic content: THD/Chebyshev harmonics on the "sine," rounded triangle peaks, exponential saw ramp + soft capacitor reset, tanh-softened square/pulse edges + duty asymmetry. Turns clean-digital into vintage-transistor/Moog/Roland coloration — a continuously dialable warmth axis most VCOs lack. | MEDIUM | Verbatim `Waveshape` character math. **Caveat:** character mostly *softens* edges (rounded peaks, soft edges → fewer highs → aliasing-friendly), but saw exponential curvature + duty asymmetry can *add* harmonics. The polyBLEP must band-limit the **characterized** waveform, not the ideal shape — character and anti-aliasing are coupled, not independent. |
| **DRIFT as audible analog instability** | On a VCO, "drift" becomes: slow multi-timescale **pitch wander** (the 0.05–2 Hz OU layers → thermal/vintage out-of-tune-ness), tiny per-sample **phase jitter** (a subtle non-tonal shimmer that keeps the tone from being sterile), slow **DC-offset wander**, and per-instance **component spread** (each instance detuned/timbrally unique → instant analog unison when stacked). The headline "sounds alive" differentiator. | MEDIUM | Verbatim `DriftEngine::step()`. **Critical calibration flag:** authority constants are tuned for an LFO (max 7.5% free / 2% clocked of *rate*). 7.5% of *pitch* ≈ **~125 cents** — a wild, unmusical warble on a VCO. The VCO has no "clocked" mode, so it would take the 7.5% branch. **The VCO needs its own, much smaller pitch-drift authority** (real analog drift ≈ ±5–30 cents ⇒ ~0.3–1.8%), audition-gated. Phase-jitter and DC authorities likely also want VCO-specific values. This is a real requirement, not a copy-paste. |
| **Per-instance analog "fingerprint" (component spread)** | Because each module seeds its own spread, no two instances are identical — real hardware behavior, and the reason stacking instances = free unison. | LOW | Already produced by `setSpreadSeed()`; no new DSP beyond seeding on VCO construction. Reinforces the mono-is-OK story. |
| **Note / pitch readout on the CRT** | The LFO shows BPM; the VCO can show resolved note + frequency (e.g. "A4 · 440 Hz") in a display pill — genuinely useful for tuning and a natural idiom swap. | LOW–MEDIUM | Display-only; reads the pitch the DSP already computes. |

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative / Decision |
|---------|---------------|-----------------|------------------------|
| **Per-shape simultaneous outputs (sin/tri/saw/sqr jacks)** | It's what Fundamental/Bogaudio/EvenVCO do; users pattern-match to "normal VCO." | Destroys the identity — the morph crossfade *is* the product; separate outs make MORPH meaningless and blow up the panel. | **Keep the single morphed ±5 V output.** Genre-correct for a morph/macro oscillator (Plaits/Surge do the same). A design pillar, not a shortcoming. |
| **Wavetable mode** | "Add a wavetable and it's a Serum-killer." | Different synthesis paradigm; dilutes the analog identity; huge scope. Already PROJECT out-of-scope. | Out. The analog morph *is* the wave palette. |
| **Built-in sub-oscillator** | Common on bass VCOs. | Adds a knob/jack and a second core; dilutes the three-knob focus; panel cost. | Out. Patch a second instance an octave down (spread gives free detune). |
| **Polyphony (v2.0)** | Modern default; chords from one module. | 16× CPU, 16× drift engines, per-voice display ambiguity; plugin-wide decision. | Out for v2.0; document mono clearly; strongest v2.1+ reconsideration. Duplicate instances for unison now. |
| **Through-zero FM** | Deep metallic/bell FM; Bogaudio has it. | DC-offset + phase-sign handling + interacts with polyBLEP; genuine complexity. | **Deferred to v2.1** (PROJECT Future). v2.0 ships expo FM only. |
| **Phase distortion (Casio-CZ)** | Distinct, loved synthesis flavor. | A whole separate synthesis mode, not a knob; better as its own scoped increment. | **Deferred to v2.1.** |
| **Off/2×/4× oversampling switch** | The "serious anti-aliasing" toggle. | Anti-aliasing infra; prove morph-aware polyBLEP first before adding oversampled paths. | **Deferred to v2.1.** Ship clean single-rate polyBLEP first. |
| **Linear FM mode (v2.0)** | Pairs with expo for classic FM. | Adds a mode/jack; its natural partner (TZFM) is already deferred; keep v2.0 lean. | Expo FM only for v2.0. Reconsider alongside TZFM in v2.1. |
| **Separate PWM knob/CV** | Users expect a PW knob on a square. | PWM is already *inside* morph (square→narrow-pulse region); a separate knob double-encodes duty and confuses the sweep. | Out — pulse width is the morph tail, by design (matches the LFO decision). |
| **Amplitude envelope / VCA / built-in FX / MIDI / quantizer / scope-analyzer** | "One module does everything." | Oscillators oscillate; these are upstream/downstream jobs. All already PROJECT out-of-scope. | Out. |
| **Tracking-error modeling toggle** | Authentic non-linear tracking. | Realism polish that fights *correct* pitch tracking; low priority. | **Deferred to v2.1.** Get accurate 1V/oct first; drift already provides "alive" wander. |

## Feature Dependencies

```
1V/oct tracking (exp2 path)
    └──required by──> Exponential FM   (both sum into the same pitch/exp2 path)
    └──required by──> Coarse/Fine tune (pitch summation)

Morph-aware polyBLEP anti-aliasing  ── the linchpin ──
    ├──required by──> MORPH as audio timbre   (continuous crossfade must be band-limited)
    ├──coupled-with─> CHARACTER                (must band-limit the *characterized* edges, not ideal shapes)
    └──coupled-with─> Hard sync                (sync discontinuity needs its own BLEP correction)

DRIFT at audio rate
    └──requires──> VCO-specific drift authority recalibration
                   (LFO's 7.5% pitch authority ≈ 125 cents = unmusical; needs ~0.3–1.8%)

Component spread (per-instance seed)
    └──enhances──> stacking instances = analog unison  (mitigates mono limitation)

Single morphed output ──conflicts with──> per-shape outputs (mutually exclusive by design)
Polyphony ──conflicts with──> per-voice DRIFT engine cost (why v2.0 is mono)
```

### Dependency notes
- **Anti-aliasing is the critical path.** MORPH (differentiator), CHARACTER (differentiator), and hard sync (table stake) all depend on the same morph-aware polyBLEP. It should be its own early phase; the three features layer on top. This is the milestone's dominant risk.
- **FM / coarse / fine are cheap** once the exp2 pitch path is wired — all pitch summation into one place, reusing the LFO's proven `exp2_taylor5`.
- **DRIFT is a copy of the engine + a calibration task**, not new DSP. The calibration (finding a musical pitch-drift authority, audition-gated) is the real work and must not be skipped by reusing LFO constants.

## MVP Definition

### Launch With (v2.0)
- [ ] **1V/oct tracking** (exp2 path) — the defining VCO contract
- [ ] **Coarse + fine tune** — table stakes, cheap once the pitch path exists
- [ ] **Morph-aware polyBLEP anti-aliasing** — the invisible table stake; everything timbral depends on it
- [ ] **MORPH / CHARACTER at audio rate** — the core identity (band-limited)
- [ ] **DRIFT at audio rate with VCO-specific authority** — headline differentiator + required recalibration
- [ ] **Hard sync input** — table stake; coupled to polyBLEP
- [ ] **Exponential FM in + attenuverter** — table stake (the attenuverter is not optional)
- [ ] **±5 V single morphed output** with saturation — table stake + design pillar
- [ ] **Static single-cycle CRT preview + note/Hz readout** — carried expectation, no live scope

### Add After Validation (v2.1)
- [ ] Through-zero FM — deep FM timbres once expo FM + polyBLEP are proven
- [ ] Off/2×/4× oversampling — after single-rate polyBLEP is trusted
- [ ] Linear FM mode — natural partner to TZFM
- [ ] Tracking-error modeling (right-click) — realism polish on accurate tracking

### Future Consideration (v2.x+)
- [ ] Phase distortion (Casio-CZ) — a distinct synthesis mode, its own increment
- [ ] Polyphony — only if the *plugin-wide* mono decision is revisited; requires per-voice drift engines

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| 1V/oct tracking | HIGH | MEDIUM | P1 |
| Morph-aware polyBLEP | HIGH (invisible) | HIGH | P1 |
| MORPH audio-rate | HIGH | LOW (engine reuse) | P1 |
| CHARACTER audio-rate | HIGH | MEDIUM (aliasing-coupled) | P1 |
| DRIFT audio-rate + recalibration | HIGH | MEDIUM (calibration, not DSP) | P1 |
| Coarse/Fine tune | HIGH | LOW | P1 |
| Expo FM + attenuverter | HIGH | MEDIUM | P1 |
| Hard sync | MEDIUM–HIGH | MEDIUM (BLEP-coupled) | P1 |
| ±5 V single output + saturation | HIGH | LOW | P1 |
| Static CRT preview + note readout | MEDIUM | MEDIUM | P1 |
| Through-zero FM | MEDIUM | HIGH | P3 (v2.1) |
| Oversampling switch | MEDIUM | MEDIUM | P3 (v2.1) |
| Linear FM | LOW–MEDIUM | LOW | P3 (v2.1) |
| Tracking-error model | LOW | LOW | P3 (v2.1) |
| Phase distortion | MEDIUM | HIGH | P3 (v2.x) |
| Polyphony | HIGH | HIGH | P3 (plugin decision) |

## Controls / Panel (18HP Forge Noir, LFO-derived)

The VCO maps almost 1:1 onto the LFO's shipped 18HP layout — same 5 main knobs, same display, same "trimpots-over-jacks" bottom convention — by **swapping LFO-specific I/O for VCO I/O**. A low-risk panel evolution, not a redesign.

**Main knobs (5 — identical count to the LFO):**
- MORPH, CHARACTER, DRIFT (the three engine axes — unchanged)
- **COARSE** (replaces LFO RATE) — wide tune, ±~5 octaves, optional right-click octave/semitone snap
- **FINE** (replaces LFO PHASE) — ±1 semitone detune

**Jacks + attenuverters (bottom two rows, trimpots above jacks):**
- **V/OCT in** (new — the primary input)
- **FM in + FM attenuverter** (the one net-new control vs the LFO)
- **SYNC in** (new — replaces CLK/RESET)
- **MORPH CV + atten**, **CHARACTER CV + atten**, **DRIFT CV + atten** (unchanged — LFO already has these trimpots)
- **OUT** (single morphed ±5 V — unchanged)

**Panel-fit assessment:** MEDIUM density, fits. Jack count rises slightly (V/OCT, FM, SYNC, 3× CV, OUT = 7 jacks + 4 attenuverters) vs the LFO, but the LFO already carried CLK/FM/RESET/phase-CV/3× main-CV in the same 18HP with room. The FM attenuverter is the only genuinely new widget to place. No panel expansion needed.

## Display: what's realistic at audio rate

**Recommendation: keep the static single-cycle morph preview (like the LFO), and drop/repurpose the spinning phase dot.**

- A **live oscilloscope trace is not viable** — at audio rate the waveform completes hundreds–thousands of cycles per rendered frame; a real trace would be a blur or need heavy sync/decimation for no benefit.
- The **single-cycle preview is exactly right**: render one cycle of the current morphed+characterized shape, updating in real time as MORPH/CHARACTER/DRIFT (and their CV) move — the same code path and lock-free double-buffer the LFO already uses. It communicates timbre, which is the whole point.
- **The phase dot should be dropped or decoupled.** At real pitch it spins far too fast to read (a blur). Either omit it, or animate a cosmetic indicator at a fixed visually-pleasant rate independent of actual frequency (honest cosmetic, not a measurement).
- **DRIFT visualization carries over** — the display can subtly show drift-induced deformation exactly as the LFO does (the atomic `displayDrift` bridge already exists).
- **Swap the readout pill:** LFO shows BPM/ratio; VCO shows **resolved note + frequency** ("A4 · 440 Hz"). Useful for tuning, idiomatic, reuses the existing pill/HUD rendering.
- Guardrail: this stays a *preview*, not a scope/analyzer (PROJECT out-of-scope: "display is shape preview, not measurement tool").

## Competitor Feature Analysis

| Feature | Fundamental / Bogaudio | Befaco EvenVCO | Plaits/Braids (Audible) / Surge XT | Forge Analog VCO (our approach) |
|---------|------------------------|----------------|-------------------------------------|----------------------------------|
| Waveform outputs | simultaneous multi-jack | multi-jack + Even | **single** (timbre = knob/model) | **single morphed** ±5 V |
| Timbre control | pick a jack | pick a jack | MODEL/morph knob | continuous MORPH knob (+CV/atten) |
| Character/warmth | fixed | fixed (even harmonics) | model-dependent | continuous CHARACTER axis (unique) |
| Analog instability | none/subtle | analog by nature | none (digital) | explicit DRIFT axis (unique) |
| FM | expo + linear (+TZ Bogaudio) | linear | expo (+TZ some) | expo only (v2.0); TZ deferred |
| Hard sync | yes | yes | yes | yes |
| Polyphony | yes | **no (mono)** | yes | **no (mono, v2.0)** — per-voice drift cost |
| Anti-aliasing | bandlimit + oversample | analog | bandlimit/oversample | morph-aware polyBLEP (single-rate v2.0) |
| Display | minimal/none | none | some (Surge) | real-time single-cycle CRT (differentiator) |

## Sources

- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) — 1V/oct, C4 = 0V baseline, ±5 V / 10 Vpp output, saturation guidance, polyphony recommendation (up to 16 ch keyed off V/oct) — HIGH
- [Bogaudio VCO — VCV Library](https://library.vcvrack.com/Bogaudio/Bogaudio-VCO) — simultaneous outs, expo+linear TZFM, PWM, poly, bandlimit+oversample — HIGH
- [BogaudioModules README](https://github.com/bogaudio/BogaudioModules/blob/master/README.md) — HIGH
- [Befaco Even VCO — vendor page](https://www.befaco.org/even-vco/) and [VCV Library](https://library.vcvrack.com/Befaco/EvenVCO) — mono, 7–10 octave tracking, hard sync, linear FM, PWM, Even output — HIGH
- [Instruō Cš-L manual](https://www.instruomodular.com/wp-content/uploads/2019/09/Cs-L-Manual-A5.pdf) — complex morphing oscillator, wavefolder/symmetry biasing, multi-out — MEDIUM
- [Surge XT VCV Rack manual](https://surge-synthesizer.github.io/rack_xt_manual/) — multi-model single-output oscillators — MEDIUM
- Engine source read directly: `src/dsp/Waveshape.hpp`, `src/dsp/DriftEngine.hpp`, `docs/engine-concept.md`, `.planning/PROJECT.md` — HIGH (authoritative for reuse/dependency/calibration claims)

---
*Feature research for: VCV Rack analog morphing VCO (v2.0, `ForgeAudio-AnalogSeries`)*
*Researched: 2026-07-20*
