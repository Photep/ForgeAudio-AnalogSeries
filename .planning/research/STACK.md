# Stack Research

**Domain:** Band-limited analog morphing VCO for VCV Rack 2 (audio-rate extension of an existing morph/character/drift engine)
**Researched:** 2026-07-20
**Confidence:** HIGH (anti-aliasing technique selection, pitch conventions, sync); MEDIUM (exact character↔BLEP interaction tuning — a per-implementation judgment call, flagged below)

> This is a **technique/algorithm** stack, not a package stack. Nothing new is installed. The "stack" is a set of DSP techniques and a small amount of new **Rack-free, C++11-clean, header-only** code (`src/dsp/*.hpp`) that mirrors the existing `LfoCore`/`Waveshape` core, plus a thin Rack shell (`src/AnalogVCO.cpp`). Every recommendation is chosen to hold under three hard constraints: **(1) strict `-std=c++11 -pedantic-errors`**, **(2) zero Rack-SDK includes in the DSP core** (so `make test` stays link-free), and **(3) bit-exact golden `.f32` regression across GCC/clang with `-ffp-contract=off`**.

---

## The crux answer up front: how to band-limit a *continuous morph crossfade*

The morph output is a **linear weighted sum** of shapes (see `Waveshape::morphedWave`):

```
result = shapes[seg] + frac*(shapes[seg+1] - shapes[seg])   (+ linear bleed term, then /(1+bleedIntensity))
```

The decisive fact, confirmed across the DSP literature: **BLEP/BLAMP correction is linear and superposes, and it can be applied to a discontinuity of any order in any waveform *as long as the position and magnitude of the discontinuity are known*** (Valimaki/Pekonen; KVR/DAFx consensus). Because the morph is a weighted sum, the **effective discontinuity at each known phase location is just the same weighted sum of the per-shape jump magnitudes**. So you do **not** band-limit "a mystery waveform" — you band-limit a small, fixed set of analytically-known discontinuity sites, each scaled by the current morph weights.

**Discontinuity map of the 5 shapes** (phase ∈ [0,1), falling-saw convention):

| Shape | Value-step (0th-order) discontinuities → **polyBLEP** | Slope (1st-order) discontinuities → **polyBLAMP** |
|-------|-------------------------------------------------------|---------------------------------------------------|
| Sine | none (C∞) | none |
| Triangle | none | corner at φ=0 (peak) and φ=valley≈0.5 |
| Saw (falling) | **+2** at φ=0 wrap | none (constant slope) |
| Square | **±2** at φ=0 and φ=duty(0.5) | none |
| Pulse | **±2** at φ=0 and φ=duty (duty sweeps 0.50→0.05) | none |

**Algorithm (per sample):**
1. Compute the naive characterized `morphedWave(p, morph, character, bleedLfo)` exactly as today.
2. Advance `phase`. Detect which discontinuity site(s) the phase crossed **this sample** (wrap at 0; duty edge; triangle corners). Usually 0–1 fire per sample; up to 2 near a narrow pulse.
3. For each crossed site, compute the **morph-weighted jump** `h = Σ wᵢ·jumpᵢ` from the analytic per-shape steps above (from the same `segment`/`frac`/`pulseDuty` already computed). Compute the sub-sample fractional position `t` of the crossing from `phase` and `deltaPhase`.
4. Add `h · polyBLEP(t, dt)` to the output (value steps) and `slope_change · polyBLAMP(t, dt)` for triangle corners.
5. **Scale each correction by the same `1/(1+bleedIntensity)` factor** the naive path applies (the bleed normalization is linear — the BLEP must ride through it to stay consistent).

That is the whole trick. It is O(1) per discontinuity, table-free, and every operation is `+ - * /` on floats → **bit-stable and C++11-trivial**.

**Character interaction (the one genuine judgment call — MEDIUM confidence).** `character` softens edges over a *fixed fraction of the cycle* (~8% tanh width, rounded peaks, soft saw reset). Fraction-of-cycle softening does **not** actually band-limit at audio rate — at 2 kHz on 48 kHz, 8% of a cycle is ~2 samples; at 8 kHz it's well under a sample — so it aliases just like a hard edge, only with a slightly smaller instantaneous jump. **Recommendation for lean v2.0:** drive polyBLEP/polyBLAMP from the **pristine (character-independent) analytic jumps**, weighted by morph, applied at full authority, on top of the characterized naive term. Rationale: (a) worst-case aliasing is at `character=0` (pure digital edges) and full-strength BLEP is exactly right there; (b) `character` is a *lowpass-ish coloration* that **reduces** harmonic energy, so any slight over-correction at high character just adds a hair more HF rolloff — benign and inaudible, whereas under-correction would leave audible alias tones. Erring toward the pristine jump is the safe direction. A magnitude-matched refinement (measure the residual step after characterization) is a v2.1 nicety, not a v2.0 requirement.

---

## Recommended Stack

### Core Technologies

| Technique | Version / Source | Purpose | Why Recommended |
|-----------|------------------|---------|-----------------|
| **polyBLEP** (2-sample polynomial band-limited step) | Valimaki & Huovilainen 2007; Pekonen 2010 formulation | Anti-alias the 0th-order (value) discontinuities of saw/square/pulse in the morph sum | **Table-free closed form** → no startup impulse table (see minBLEP rejection below), so it adds **zero** Rack-SDK coupling and **zero** cross-compiler table-generation risk. Pure `+ - * /` → trivially C++11 and bit-stable under `-ffp-contract=off`. O(1)/disc, ~a dozen flops. Correctly composes with the linear morph sum. |
| **polyBLAMP** (2-sample band-limited ramp / 1st-order residual) | Esqueda/Bilbao/Valimaki, DAFx-2016 "Rounding Corners with BLAMP" | Anti-alias the **triangle's slope corners** (peak + valley), which polyBLEP cannot fix | The triangle contributes **no value jump** but two slope breaks; without BLAMP the triangle end of the morph aliases on bright settings. Same table-free, closed-form, bit-stable properties as polyBLEP. Residual ∝ `d⁴/6` (sample before corner), `(1-d)⁴/6` (sample after), scaled by the slope change × dt. |
| **Rack V/Oct pitch law** | `dsp::FREQ_C4 = 261.6256f`; `freq = FREQ_C4 · 2^pitch` | 1V/oct tracking | The VCV standard. C4 = 0 V reference; +1 V = +1 octave. Exactly how Fundamental VCO computes pitch. |
| **`forge::exp2_taylor5`** (already in `RackCompat.hpp`) | Rack `approx.hpp` degree-5 minimax, verified bit-identical | The `2^pitch` evaluation for pitch **and** exponential FM | **Reuse, do not replace.** This is precisely the function Fundamental VCO uses for VCO pitch. Integer octaves are exact (the `exp2Floor` bit trick); fractional error is ~1e-6 relative (≪0.002 cents) across the whole audio range and well past +10 V. It is a **VCO-grade** approximation, not just an LFO convenience — **no wider-range exp is needed** (see Q2 verdict below). Reuse is also mandated by a documented **bit-identity landmine (Pitfall 2)**: the golden path depends on this exact polynomial, so the VCO must use the same function, not `std::exp2`/`std::pow`. |
| **Classic hard-sync BLEP** | Standard technique | Band-limit the discontinuity created by phase reset on the master's rising edge | Sync *is* a discontinuity (`out(pre-reset) → out(new phase)`). Reset alone aliases badly. The fix is one more polyBLEP, scaled by the reset jump, at the sub-sample sync time — same machinery as row 1. |
| **`VcoCore.hpp`** (new, mirrors `LfoCore.hpp`) | New Rack-free header | Per-sample orchestrator: pitch→freq→phase→morph→BLEP→sync | Mirror the proven `LfoCore` shape (POD `Inputs`, `step()`, `Telemetry`) so the shell/test harness/golden pattern carries over unchanged. Reuses `Waveshape`, `DriftEngine`, `RackCompat` **directly** — no duplication (per PROJECT.md milestone structure). |

### Supporting Libraries

| Component | Source | Purpose | When to Use |
|-----------|--------|---------|-------------|
| `forge::polyBLEP(t, dt)` + `forge::polyBLAMP(t, dt)` | **New, add to `RackCompat.hpp`** (or a new `dsp/Blep.hpp`) | The two residual kernels | Always — the anti-aliasing core. ~15 lines total, header-only, `inline`. |
| `forge::OnePole` (exists) | `RackCompat.hpp` | Reuse for freq slew / any smoothing | As in the LFO. Also the building block for a DC blocker (below). |
| **DC blocker** (1st-order highpass ≈ 5–20 Hz): `hp = x − onepole.process(x)`, or `y = x − x_prev + R·y_prev` | New tiny helper | Remove DC that a VCO output legitimately must not have | Narrow pulse (duty→0.05) and asymmetric duty/character introduce strong DC. The LFO *keeps* DC on purpose (`dcOffsetV`); the **VCO should strip it** at the output. Compute `R = exp(-2π·fc/fs)` with `std::exp` **once per sample-rate change**, not per sample, to keep the hot path table-free and bit-stable. |
| `SchmittTrigger` (exists) | `RackCompat.hpp` | Sync-input edge detection; also any CV gating | Hard-sync rising-edge detect (use thresholds consistent with the existing reset trigger, e.g. 0.1/1.0 V). |
| `DriftEngine` (exists, reuse) | `dsp/DriftEngine.hpp` | Multi-timescale OU pitch drift at audio rate | Reuse as-is, but **scale drift authority down** for audio rate (drift that reads as "alive" at LFO Hz is gross detuning at 440 Hz). Drift is multiplicative on `deltaPhase` — retune the depth, not the mechanism. |
| `Xoroshiro128Plus` (exists) | `RackCompat.hpp` | Per-instance drift/spread seeding | Reuse the LFO's seeding + spread-seed serialization pattern verbatim. |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| `make strict` (`-std=c++11 -pedantic-errors`) | Gate every new VCO source against the VCV toolchain | **Run before any commit.** No `inline constexpr` variables, no `[[maybe_unused]]`; any ODR-used `static constexpr` array (a jump table, polyBLEP coefficients) needs an **out-of-line definition**. Prefer function-local `const float a[] = {...}` to sidestep ODR entirely. |
| `make test` + headless `BlockDriver` + golden `.f32` | Prove `VcoCore` bit-exact and alias-bounded | Extend the harness: capture a VCO golden fixture and add **spectral invariants** — e.g. play a high note and assert alias energy below a threshold, and assert the pre-BLEP vs post-BLEP paths differ only in the expected band. |
| GitHub Actions CI (MinGW link gate) | Catch C++11/ODR issues clang masks locally | Already mirrors the library. The new `.cpp` must link under MinGW GCC. |
| `-ffp-contract=off` (already set) | Cross-platform bit stability | **Load-bearing for the golden.** polyBLEP/BLAMP are FMA-friendly polynomials; without this flag GCC/clang contract `a*b+c` differently and break the golden. Keep it. |

## "Installation"

No packages. The work is:

```
src/dsp/VcoCore.hpp     # NEW — mirrors LfoCore.hpp (pitch, phase, morph, BLEP, sync, drift)
src/dsp/Blep.hpp        # NEW — forge::polyBLEP / forge::polyBLAMP (or fold into RackCompat.hpp)
src/AnalogVCO.cpp       # NEW — thin Rack shell: params/inputs/outputs, args.sampleTime/Rate, display
plugin.cpp              # +1 addModel(modelAnalogVCO)
plugin.json  modules[]  # +1 entry (second module, same plugin/slug/submission #929)
tests/ + tools/         # extend BlockDriver + capture a VCO golden fixture
```

Reference polyBLEP / polyBLAMP kernels (canonical 2-sample forms; adapt sign to jump direction):

```cpp
// t = fractional phase-distance from the discontinuity, in [0,1); dt = deltaPhase (normalized)
inline float polyBLEP(float t, float dt) {
    if (t < dt)            { t /= dt;          return t + t - t*t - 1.f; } // just AFTER the step
    else if (t > 1.f - dt) { t = (t - 1.f)/dt; return t*t + t + t + 1.f; } // just BEFORE next
    return 0.f;
}
// BLAMP residual for a slope break of size 'ds' (change in dy/dphase) — scale by ds*dt:
inline float polyBLAMP(float t, float dt) {
    if (t < dt)            { t /= dt;          float u = t - 1.f; return -(u*u*u*u)/6.f * dt; }
    else if (t > 1.f - dt) { t = (t - 1.f)/dt; float u = t + 1.f; return  (u*u*u*u)/6.f * dt; }
    return 0.f;
}
```
(Exact constants/signs get pinned during implementation against the golden; shapes above are the standard Valimaki/DAFx forms. Keep coefficients as function-local `const` to avoid ODR/out-of-line chores under `-pedantic-errors`.)

## What comparable VCV / synth oscillators actually do

| Oscillator | Anti-aliasing approach | Takeaway for us |
|------------|------------------------|-----------------|
| **VCV Fundamental VCO-1/VCO-2** | **minBLEP** (`dsp::MinBlepGenerator<16, 32>`), switched *away from oversampling* in Rack 2; pitch via `FREQ_C4 · approxExp2_taylor5(pitch)` | Confirms our pitch law and our `exp2_taylor5` choice. They accept minBLEP's precomputed table because they *are* Rack (SDK-coupled). We're Rack-free in the core, so we diverge to polyBLEP. |
| **Befaco EvenVCO** | Uses the SDK `MinBlepGenerator` for saw/square edges | Same story — fine for an SDK-coupled module, wrong for a bit-stable Rack-free core. |
| **Bogaudio** | **Hybrid: band-limiting + oversampling** | Validates that a mixed strategy is normal; but oversampling is explicitly **deferred to v2.1** for us. polyBLEP-only is the lean choice. |
| **Surge XT "Classic"** | Per-voice **BLEP / oversample** internally | Confirms BLEP-family is the industry-standard route for morph/multi-shape oscillators. |
| **Polygonal/phaseshaping morph oscillators (DAFx-2017)** | polyBLEP applied at *known* discontinuity positions of a parametric shape | **Direct precedent for our crux** — band-limit by locating discontinuities analytically and injecting scaled polyBLEP; exactly the weighted-sum approach. |

## Answers to the four specific questions

**Q1 — Anti-aliasing for the continuous morph crossfade.** Use **polyBLEP (value steps) + polyBLAMP (triangle corners)**, driven by morph-weighted analytic jump magnitudes at the fixed discontinuity sites, scaled through the bleed normalization. See "The crux answer" above. Reject minBLEP/BLIT/DPW/oversampling for v2.0 (see What NOT to Use). Complexity: O(1) per discontinuity, 0–2 discontinuities/sample, table-free.

**Q2 — 1V/oct pitch.** `pitch_volts = coarse + fine + octaveKnob + V/Oct_input (+ FM)`, summed in volts; `freq = dsp::FREQ_C4 · forge::exp2_taylor5(pitch_volts)`. Params and the V/Oct input **sum** (Rack convention). **`forge::exp2_taylor5` is accurate enough for a VCO across ~20 Hz–20 kHz and past +10 V — verdict: no wider-range exp needed.** It's the exact function Fundamental VCO uses; integer octaves are exact via the bit trick, fractional error ≪0.002 cents. It must be reused (not swapped for `std::exp2`) to preserve the documented golden bit-identity (Pitfall 2). Clamp the resulting `freq` to just under Nyquist (`min(freq, 0.5·sampleRate·0.99)`) so extreme pitch/FM can't push the fundamental past Nyquist.

**Q3 — Hard sync.** `SchmittTrigger` on the sync input; on a rising edge compute the sub-sample crossing time, capture the current output as the pre-reset value, reset `phase` to the fractional overshoot (`phase = syncFracRemaining · deltaPhase`, preserving sub-sample timing — do **not** snap to exactly 0), and **insert a polyBLEP scaled by `out_preReset − morphedWave(newPhase)` at the sync fractional time**. This is the classic hard-sync BLEP and reuses the Q1 machinery. (A BLAMP for the slope change is optional; the step BLEP is the audible fix and is sufficient for lean v2.0.)

**Q4 — Rack DSP helpers worth using.** Use `args.sampleTime` for `deltaPhase` and BLEP fractional math, and `args.sampleRate` for the Nyquist clamp and DC-blocker coefficient (recompute on sample-rate change, exactly as the LFO handles `sampleTime`). **Do NOT pull `dsp::MinBlepGenerator` or `dsp::TRCFilter` into the DSP core** — they're SDK-coupled and would break the Rack-free `make test`; reimplement the trivial equivalents in `forge::` (a DC blocker from `OnePole`; polyBLEP instead of MinBlep). `dsp::Decimator`/`Upsampler` (oversampling) are explicitly deferred to v2.1.

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| polyBLEP + polyBLAMP | **minBLEP** (`dsp::MinBlepGenerator`) | If the core could link the SDK and you wanted the last few dB of alias suppression. Rejected: needs a precomputed windowed-sinc impulse table (Rack builds it with libm/pffft at startup) → **SDK coupling breaks Rack-free `make test`, and startup table generation risks cross-compiler bit drift** vs the golden. Also O(Z) taps + ring buffer per discontinuity. |
| polyBLEP + polyBLAMP | **2×/4× oversampling + halfband decimation** | Best when the shaping is a genuine nonlinearity you can't BLEP analytically (the tanh character edges *themselves*, or through-zero FM). **Deferred to v2.1** by scope decision; needs `dsp::Decimator` or a `forge::` polyphase FIR. It's *complementary* to polyBLEP, not a replacement. |
| polyBLEP + polyBLAMP | **DPW / differentiated parabolic waveform** | Elegant for a pure saw/square, but generalizes poorly to a *runtime crossfade of 5 differently-shaped waves* (per-shape antiderivatives + N-difference operator + startup transients). polyBLEP composes with the weighted sum far more cleanly. |
| polyBLEP + polyBLAMP | **BLIT (band-limited impulse train) + integration** | Needs leaky integrators with DC/drift management and doesn't map naturally onto the morph crossfade. Legacy; polyBLEP superseded it here. |
| `forge::exp2_taylor5` | `std::exp2` / `std::pow` | Never in the golden path — breaks Pitfall-2 bit-identity and is slower. `std::exp2` is acceptable only in non-golden init code (e.g. a DC-blocker coefficient computed once). |
| Reuse `DriftEngine` at reduced authority | New audio-rate drift model | Only if OU drift proves musically wrong at audio rate. Start by retuning depth; the mechanism is proven. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| `dsp::MinBlepGenerator` in `VcoCore.hpp` | SDK include → breaks the Rack-free core / `make test`; startup impulse-table generation threatens golden bit-stability across GCC/clang | `forge::polyBLEP` (table-free, closed-form, in-core) |
| `dsp::TRCFilter`, `dsp::Decimator`, other `rack::dsp::*` in the core | Same SDK-coupling problem; the whole point of `src/dsp/*.hpp` is zero Rack includes (Pitfall 1) | `forge::OnePole`-based helpers in `RackCompat.hpp` |
| `std::exp2` / `std::pow` for pitch/FM | Breaks documented golden bit-identity (Pitfall 2); Fundamental VCO itself avoids it | `forge::exp2_taylor5` |
| `inline constexpr` variables, `[[maybe_unused]]`, in-class `static constexpr` array *definitions* | Fail `-std=c++11 -pedantic-errors` / MinGW link (the exact class of bug that got v2.0.0 rejected) | Function-local `const` arrays; out-of-line defs only where unavoidable |
| `-ffast-math` / dropping `-ffp-contract=off` | Reorders/contracts polyBLEP FMAs → golden diverges GCC↔clang | Keep the existing test flags verbatim; capture the VCO golden with the same flags |
| Oversampling infra, through-zero FM, phase distortion, tracking-error model | **Out of lean v2.0 scope** — deferred to v2.1; adding now re-introduces the hard problems this milestone was scoped to de-risk | Ship polyBLEP-only core first; layer oversampling in v2.1 |
| Snapping sync reset to exactly `phase = 0` | Destroys sub-sample sync timing → sync itself aliases even with a BLEP | Reset to fractional overshoot `syncFracRemaining · deltaPhase` |
| Discrete per-shape "BLEP the active shape" switch | Misses the crossfade region — during a morph, *two* shapes' discontinuities are simultaneously partially present | Weighted-sum jump `h = Σ wᵢ·jumpᵢ` across all shapes at each site |

## Stack Patterns by Variant

**If aliasing is still audible at extreme brightness (square/pulse, high notes) with polyBLEP alone:**
- Don't jump to minBLEP first. Verify polyBLAMP is firing on the triangle corners and the sync BLEP is scaled correctly.
- The proper escalation is **v2.1 oversampling (2×/4×)** layered on top — polyBLEP + light oversampling is the Bogaudio-style hybrid and the intended growth path.

**If DC offset appears at narrow pulse / high character:**
- Add the 5–20 Hz `forge::` DC blocker at the VCO output (NOT in the LFO path — the LFO intentionally preserves DC via `dcOffsetV`).

**If drift sounds like detuning rather than "alive" at audio rate:**
- Reduce `DriftEngine` authority for the VCO (keep the OU mechanism, scale the depth into a few-cents range), mirroring how the LFO already scales drift authority by mode (2% clocked vs 7.5% free).

## Version / Constraint Compatibility

| Element | Constraint it must satisfy | Notes |
|---------|----------------------------|-------|
| `VcoCore.hpp`, `Blep.hpp` | `-std=c++11 -pedantic-errors`, zero Rack includes | Header-only, `inline` free functions or struct members; function-local `const` arrays |
| polyBLEP/BLAMP math | Golden bit-stability, `-ffp-contract=off` | Pure `+ - * /`; no libm in the hot path |
| `forge::exp2_taylor5` reuse | Pitfall-2 bit-identity | Same polynomial / Horner order as the LFO FM path |
| `plugin.json` version | VCV MAJOR = Rack major = 2 | Second module is a `modules[]` entry, same slug/submission #929 (a comment, not a new issue) |
| `args.sampleRate` handling | 44.1/48/96 kHz golden coverage | Recompute Nyquist clamp + DC-blocker coeff on rate change, like the LFO's `sampleTime` handling |

## Sources

- [VCV MinBlepGenerator API](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1MinBlepGenerator) — `insertDiscontinuity(p, x)` with `-1<p<=0`, `<Z,O>` = window half-width / impulse oversampling (Fundamental uses `<16,32>`); table-based ⇒ SDK-coupled. HIGH.
- [VCV minblep.hpp source](https://vcvrack.com/docs-v2/minblep_8hpp_source) + [Fundamental VCO2 library page](https://library.vcvrack.com/Fundamental/VCO2) — Fundamental switched oversampling→minBLEP; SIN/TRI/SAW/SQR shapes. HIGH.
- [VCV Voltage Standards](https://vcvrack.com/manual/VoltageStandards) + [Plugin Dev Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial) — `FREQ_C4=261.6256`, `freq=FREQ_C4·2^pitch`, V/Oct = 1V/oct, params+inputs sum. HIGH.
- [DAFx-2016 "Rounding Corners with BLAMP" (Esqueda/Bilbao/Valimaki)](https://www.dafx.de/paper-archive/2016/dafxpapers/18-DAFx-16_paper_33-PN.pdf) — polyBLAMP for slope (triangle) discontinuities; `d⁴/6` residual. HIGH.
- [DAFx-2017 "Efficient Anti-aliasing of a Complex Polygonal Oscillator"](http://www.dafx17.eca.ed.ac.uk/papers/DAFx17_paper_100.pdf) — polyBLEP at analytically-known discontinuities of a parametric morph shape (direct precedent for the weighted-sum crux). HIGH.
- [Martin Finke — PolyBLEP Oscillator](https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/) + [metafunction — BLITs & BLEPs](https://www.metafunction.co.uk/post/all-about-digital-oscillators-part-2-blits-bleps) — canonical 2-sample polyBLEP form; "BLEP applies to any discontinuity whose position and magnitude are known." HIGH.
- [Christian Floisand — PolyBLEP sawtooth](https://christianfloisand.wordpress.com/2014/09/03/custom-pure-data-external-polyblep-sawtooth-oscillator/) + [KVR BLEP/minBLEP/ramps thread](https://www.kvraudio.com/forum/viewtopic.php?t=248390) — polyBLEP vs minBLEP tradeoffs, table-free property. MEDIUM.
- [Bogaudio Modules (GitHub)](https://github.com/bogaudio/BogaudioModules) — hybrid band-limiting + oversampling (validates the v2.1 hybrid growth path). MEDIUM.
- [VCV community — MinBlepGenerator usage / EvenVCO](https://community.vcvrack.com/t/minblepgenerator-woes/20878) — EvenVCO uses SDK MinBlep (SDK-coupled precedent). MEDIUM.
- Repo files read directly: `src/dsp/Waveshape.hpp`, `src/dsp/LfoCore.hpp`, `src/dsp/RackCompat.hpp`, `Makefile`, `.planning/PROJECT.md` — constraints, `exp2_taylor5`, morph linearity, strict/test gates. HIGH.

---
*Stack research for: band-limited analog morphing VCO (VCV Rack 2, Rack-free bit-stable core, strict C++11)*
*Researched: 2026-07-20*
