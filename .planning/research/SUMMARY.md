# Project Research Summary

**Project:** Forge Audio — Analog Series (ForgeAudio-AnalogSeries)
**Domain:** VCV Rack 2 plugin — analog-modeled morphing VCO (second module in a shipped, VCV-Library-live plugin)
**Researched:** 2026-07-20
**Confidence:** HIGH overall (all four agents converged independently on the same crux, the same shared-header risk, and the same build order)

## Executive Summary

The Forge Analog VCO is an audio-rate extension of the shipped LFO's three-knob analog engine (morph, character, drift) into a mono, single-output, morph/macro-style oscillator — architecturally closer to Plaits/Braids/Surge (timbre is a continuous knob, one output) than to a bread-and-butter multi-jack VCO like Fundamental. The engine (`Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp`) is reused verbatim; nothing in the LFO's proven identity needs reinvention. All four research agents — independently, from different angles — converged on the same single dominant risk: **band-limiting a continuous, character-deformed morph crossfade with morph-aware polyBLEP/polyBLAMP.** This is not "BLEP a sawtooth"; it is BLEP-ing a runtime-weighted sum of five shapes whose discontinuities move, multiply, and change magnitude as MORPH, CHARACTER, and bleed all vary continuously — including interior edges (square's mid-cycle step, pulse's sweeping duty edge, triangle's slope corners) that a naive "one BLEP at the phase wrap" implementation will silently miss, producing aliasing that appears only at high notes and is easy to ship undetected.

The recommended approach is: keep polyBLEP/polyBLAMP entirely table-free and Rack-free (closed-form `+ - * /`, no SDK includes, no minBLEP tables) so it stays inside the existing bit-stable, C++11-strict, golden-regression discipline; drive the BLEP magnitude from the *measured, characterized* jump (sample the shape just-before/after each discontinuity) rather than an assumed ideal ±2 step, so character's edge-softening auto-scales the correction; and treat every crossed discontinuity per sample (there can be more than one, especially at narrow pulse widths) with linear superposition rather than a single wrap-only correction. Hard sync reuses the same BLEP machinery, placed at the *master's* sub-sample wrap fraction — explicitly not routed through the LFO's 3 ms cosine crossfade, which is the wrong tool at per-cycle audio-rate sync and would mute/smear the classic sync buzz.

The second load-bearing risk, equally emphasized across STACK/ARCHITECTURE/PITFALLS, is protecting the shipped, VCV-Library-live LFO while building this: the four shared DSP headers must be treated as frozen, all new work lives in new additive files (`VcoCore.hpp`, `MorphBlep.hpp`, a second `DriftEngine` instance), and every phase that could conceivably touch shared code ends with a `make test` replay proving the LFO's `.f32` goldens are still byte-identical. The lean v2.0 scope (deferring through-zero FM, phase distortion, oversampling, and tracking-error modeling to v2.1) exists specifically to keep this milestone's hard problem — morph-aware anti-aliasing — isolated and provable before compounding it with harder DSP.

## Key Findings

### Recommended Stack

The "stack" is a set of DSP techniques, not packages: polyBLEP (Valimaki/Huovilainen/Pekonen) for the value-step discontinuities (saw wrap, square/pulse duty edges, sync reset), and polyBLAMP (Esqueda/Bilbao/Valimaki DAFx-2016) for the slope-break discontinuities (triangle corners). Both are closed-form, table-free, and pure `+ - * /`, which is what makes them compatible with `-std=c++11 -pedantic-errors`, zero-Rack-include DSP core, and bit-exact `-ffp-contract=off` goldens — the three hard constraints this repo already runs under. Pitch reuses `forge::exp2_taylor5` unchanged (verdict: it's VCO-grade, not just LFO-grade — integer octaves are exact via the bit trick, fractional error ≪0.002 cents) combined with the standard Rack `dsp::FREQ_C4 = 261.6256f` reference. `DriftEngine` is reused as a second, separately-seeded instance with re-scaled authority.

**Core technologies:**
- **polyBLEP** — anti-aliases the 0th-order (value) discontinuities of the morph crossfade — table-free, O(1)/discontinuity, composes correctly with the linear morph-weighted sum
- **polyBLAMP** — anti-aliases the 1st-order (slope) discontinuities at triangle corners — same closed-form, bit-stable properties
- **`forge::exp2_taylor5` (reused, not replaced)** — the 1V/oct + FM exponential — mandated by golden bit-identity, already VCO-accurate
- **New Rack-free headers (`VcoCore.hpp`, `MorphBlep.hpp`/`Blep.hpp`)** — mirror `LfoCore.hpp`'s proven POD-`Inputs`/`step()`/`Telemetry` shape so the test-harness and golden pattern carry over unchanged
- **Rejected: `dsp::MinBlepGenerator`, oversampling, DPW, BLIT** — all SDK-coupled, table-based, or generalize poorly to a runtime 5-shape crossfade; oversampling explicitly deferred to v2.1 as the natural escalation path if polyBLEP alone proves insufficient at extreme brightness

### Expected Features

The correct competitive frame is Plaits/Braids/Surge-style single-output morph oscillators, not multi-jack analog VCOs — this reframes "single output, mono" from a limitation into a genre-correct design pillar.

**Must have (table stakes):**
- Accurate 1V/oct tracking (≥7-10 octaves) + coarse/fine tune
- Morph-aware, band-limited output (the invisible table stake everything else depends on)
- Hard sync input
- Exponential audio-rate FM **with a dedicated attenuverter** (the one genuinely new panel control vs. the LFO)
- ±5 V single morphed output with saturation
- Real-time single-cycle CRT preview (static, regenerated off-thread — not a live scope)

**Should have (competitive differentiators, all inherited from the engine at low marginal build cost, real cost is calibration):**
- Continuous MORPH as the sole timbre axis (verbatim `Waveshape::morphedWave`)
- CHARACTER as audio-rate coloration (verbatim, but coupled to the BLEP work — must band-limit the characterized wave, not the ideal shape)
- DRIFT as audible analog instability — the headline differentiator, but requires **VCO-specific authority recalibration** (LFO's 7.5% free-mode authority ≈ 125 cents of detune at audio rate — unusable verbatim)
- Per-instance component-spread "fingerprint" — free analog unison when instances are stacked, which meaningfully offsets the mono limitation
- Note/frequency readout on the CRT (swap for the LFO's BPM pill)

**Defer (v2.1+):**
- Through-zero FM, phase distortion, Off/2×/4× oversampling, tracking-error modeling, linear FM mode — all explicitly out of lean v2.0 scope per PROJECT.md
- Polyphony — plugin-wide decision (16× drift-engine cost), not VCO-local; mono is defensible (Befaco EvenVCO precedent) and documented plainly

### Architecture Approach

The VCO instantiates the exact same three-layer pattern the LFO already proves — thin Rack shell (`AnalogVCO.cpp`) → POD `VcoInputs` boundary → pure DSP core (`VcoCore.hpp`) → shared frozen leaf headers — rather than inventing anything new. The one genuinely new subsystem is `MorphBlep.hpp`, deliberately kept as a *wrapper* that calls the frozen `Waveshape::morphedWave()` and subtracts/adds BLEP/BLAMP residuals, never as new methods added inside `Waveshape.hpp` itself. This wrapper-not-edit boundary is the single architectural decision most responsible for making the LFO non-regression guardrail tractable.

**Major components:**
1. `src/AnalogVCO.cpp` (new) — Rack shell: params/inputs/outputs, panel, display atomics, JSON seed persistence, marshals Rack I/O into `VcoInputs`
2. `src/dsp/VcoCore.hpp` (new) — pure orchestrator: pitch→FM→exp2→phase→drift→sync→`MorphBlep`→output, per-sample ordering contract mirrors the LFO's proven sequence
3. `src/dsp/MorphBlep.hpp` (new) — the band-limiting wrapper: calls `Waveshape` for the naive value, measures actual rendered jump heights at each crossed discontinuity, applies polyBLEP (steps) / polyBLAMP (slope breaks), scaled through the same bleed normalization
4. `Waveshape.hpp` / `DriftEngine.hpp` / `RackCompat.hpp` / `MathConst.hpp` — reused as-is (frozen); `DriftEngine` gets a purely additive change (configurable authority members, defaults = today's LFO literals, IEEE-bit-identical by construction)
5. `tests/VcoBlockDriver.hpp` + `tools/capture_vco_golden.cpp` + `tests/golden/vco_*.f32` (new) — parallel, never-merged harness mirroring the LFO's portable-drift-off / platform-gated-drift-on golden split

### Critical Pitfalls

1. **Single-BLEP-at-the-wrap misses interior discontinuities** (square's mid-cycle edge, pulse's sweeping duty edge, triangle's slope corners, and any character-added slope breaks) — aliasing that only appears at high notes and ships unnoticed if tested at mid pitch. Fix: enumerate *every* discontinuity crossed each sample, compute each one's morph-weighted signed jump via BLEP's linearity, apply BLEP/BLAMP per crossing (narrow pulse can produce ≥2 overlapping crossings in one sample — must sum, not overwrite).
2. **Assuming the ideal ±2 jump instead of the measured, characterized jump** — character's tanh edge-softening and saw capacitor-reset change the true discontinuity magnitude; BLEP-ing the ideal shape over- or under-corrects. Fix: sample the actual characterized waveform just-before/after each discontinuity to derive the correction magnitude (this also auto-handles the bleed-and-normalization term, which likewise changes the true jump height).
3. **Reusing LFO drift authority verbatim at audio rate** (7.5% free-mode ≈ ±125 cents) — an unmusical, broken-sounding VCO. Fix: separate, independently-seeded `DriftEngine` instance with new authority members (defaults preserving today's LFO literals bit-for-bit), retuned to a few cents for the VCO; never add/reorder RNG draws inside the shared `step()` — that shifts the LFO's random stream and breaks its drift-on goldens.
4. **Editing `Waveshape.hpp`/`DriftEngine.hpp` in place** to expose what the BLEP work "needs" — even a seemingly safe addition risks the shipped LFO's bit-exact goldens and, worse, could ship a silent behavioral regression to a live VCV Library plugin. Fix: treat the four shared headers as frozen; all new code lives in new files; any unavoidable shared-header touch must be purely additive and gated by a full LFO golden replay.
5. **Reintroducing the exact v2.0.0 rejection class** (in-class ODR'd `static constexpr` arrays, C++17-isms) in the brand-new `AnalogVCO.cpp` TU — `make strict` is syntax-only and won't catch it; only the MinGW CI **link** leg does. Fix: namespace-scope constants or out-of-line definitions, discipline enforced every phase, never tag/submit on green `make strict` alone.

## Implications for Roadmap

Based on research, suggested phase structure (all four agents converge on this ordering; ARCHITECTURE.md's Suggested Build Order is the most detailed and is used as the backbone):

### Phase 1: VCO test harness & core skeleton
**Rationale:** Establish the headless, Rack-free proving ground and prove pitch accuracy before any hard DSP — mirrors the v1.4 "test-harness-before-refactor" lesson and lets every later phase stay independently gated.
**Delivers:** `VcoInputs` POD, `VcoCore.hpp` skeleton with V/oct pitch via `exp2_taylor5`, naive (aliased-on-purpose) `morphedWave` at audio rate, `VcoBlockDriver.hpp`, tracking-accuracy + determinism invariants.
**Addresses:** 1V/oct tracking, coarse/fine tune (table stakes from FEATURES.md).
**Avoids:** Pitfall 2 (pitch-path traps: exponent overflow clamp, double-precision phase for later BLEP crossing accuracy, correct `FREQ_C4` reference, exp-domain summation) and Pitfall 5/5a (C++11/ODR + portable-golden discipline, built first per the v1.4 precedent).

### Phase 2: Drift-authority guardrail (additive, isolated)
**Rationale:** Locks in "LFO golden preserved" as early and cleanly as possible, before it can get entangled with the much larger BLEP change — this is the single most important sequencing decision from ARCHITECTURE.md.
**Delivers:** Configurable authority members on `DriftEngine` (defaults bit-identical to today's LFO literals), a separate VCO-side `DriftEngine` instance with retuned (few-cents) authority, VCO drift invariant.
**Uses:** `DriftEngine.hpp` (modified additively), `Xoroshiro128Plus` reuse pattern.
**Implements:** Architecture Pattern 3 (additive drift-authority).
**Gate (non-negotiable):** `make test` replays `freerun_*.f32` byte-identical after this change.

### Phase 3: Morph-aware polyBLEP/polyBLAMP anti-aliasing
**Rationale:** The dominant risk and linchpin, fully isolated in its own header + test now that pitch and the guardrail are proven — everything timbral (MORPH, CHARACTER, hard sync) depends on this.
**Delivers:** `MorphBlep.hpp` — BLEP for saw, then square/pulse (including overlapping-discontinuity handling at narrow duty), then BLAMP for triangle corners, driven by measured/characterized jump heights; a DFT/spectral alias-floor invariant.
**Implements:** Architecture Pattern 2 (band-limit by wrapping the frozen shape, never editing it).
**Avoids:** Pitfalls 1/1a/1b/1c (interior discontinuities, character-fooled magnitude, bleed/normalization jump, overlapping narrow-pulse kernels) — this phase should iterate until the spectrum/THD-vs-pitch harness is clean at the top two octaves.

### Phase 4: Hard sync + sync-BLEP
**Rationale:** Builds directly on Phase 3's proven BLEP residual machinery; sequencing it after anti-aliasing avoids building sync twice.
**Delivers:** `SchmittTrigger`-based sync edge detection, sub-sample reset-fraction computation, sync-BLEP reusing Phase 3's machinery placed at the master's fraction, sync-continuity invariant.
**Avoids:** Pitfall 3 (naive reset click/aliasing, and specifically NOT reusing the LFO's 3 ms cosine crossfade, which smears/mutes per-cycle audio-rate sync).

### Phase 5: Exponential FM
**Rationale:** Small and low-risk once the pitch/exp2 path exists; folds in before the shell so the FM depth/attenuverter behavior can be invariant-tested headlessly.
**Delivers:** `fmVolts·fmAtten·depth` summed into the pitch volt domain before the single `exp2_taylor5` call, FM depth/tracking test.
**Addresses:** Expo FM + attenuverter (table stake, FEATURES.md).

### Phase 6: Shell, panel, and Rack registration
**Rationale:** First point Rack is actually involved — deferred until the DSP core is already proven headless, so in-Rack UAT is validating wiring, not DSP correctness.
**Delivers:** `AnalogVCO.cpp` (enums, widget, display atomics reusing `DisplayFill`/`Anim` off-thread regeneration, seed JSON via `PatchParse` with all-zero-seed validation), `res/AnalogVCO.svg`, `plugin.hpp`/`plugin.cpp`/`plugin.json` wiring (permanent slug chosen deliberately).
**Avoids:** Pitfall 8 (display capturing live audio samples instead of off-thread regeneration) and Pitfall 6b (copying the LFO's pre-fix hostile-input/seqlock bugs instead of the corrected idioms).

### Phase 7: Golden capture, CI, strict/MinGW gate, and library resubmission
**Rationale:** Consolidates portability and ships-the-update mechanics as a final, dedicated phase — but strict/MinGW discipline is a standing canary through every prior phase, not deferred entirely to the end.
**Delivers:** `capture_vco_golden.cpp`, `vco_*.f32` fixtures (drift-off portable/3-OS, drift-on platform-gated per the LFO precedent), `test_vco_golden.cpp`, confirmed `make strict` + MinGW link leg green, manifest version bump on the 2.x line with a fresh tag, VCV Library update per #929.
**Avoids:** Pitfall 5 (ODR/C++17-ism class that sank v2.0.0 — MinGW link is the real gate, not `make strict` alone) and Pitfall 6a (slug/version/tag/update-mechanics mistakes on an already-live plugin).

### Phase Ordering Rationale

- **Guardrail before hard DSP:** the drift-authority change (Phase 2) touches a shared header and must be proven safe in isolation before the much larger, iterative BLEP work (Phase 3) could otherwise obscure a golden regression inside a wall of DSP changes.
- **Anti-aliasing before sync:** hard sync's discontinuity correction is architecturally the same BLEP machinery applied to a reset event — building it after Phase 3 avoids parallel/duplicate BLEP implementations.
- **DSP core proven headless before Rack shell:** every research agent independently favors "prove it in `make test` first" — the shell phase then only has to get wiring right, not chase DSP bugs inside NanoVG/Rack's process() loop.
- **This directly avoids the milestone's two dominant pitfall clusters:** interior-discontinuity aliasing (Pitfalls 1/1a/1b/1c) is fully contained inside Phase 3's dedicated iteration loop; LFO regression (Pitfalls 4b/6) is prevented by never touching shared headers non-additively and gating Phase 2 (the one necessary shared-header touch) with an immediate golden replay.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 3 (morph-aware polyBLEP/polyBLAMP):** MEDIUM confidence on exact character-BLEP interaction tuning and the alias-floor test threshold (target ≈ −60 dB rel. fundamental, to be refined empirically) — this is a judgment call flagged by STACK.md and ARCHITECTURE.md as needing empirical pinning once naive vs. band-limited outputs can be compared; likely worth `--research-phase` if the first iteration's spectral results are ambiguous.
- **Phase 7 (golden/CI/resubmission):** MEDIUM confidence on the precise VCV Library *feature-update* mechanics for adding a module to an already-live plugin (vs. the well-documented rejection-resubmission flow) — PITFALLS.md explicitly flags this as needing verification against current library docs at release time.

Phases with standard patterns (skip research-phase):
- **Phase 1 (skeleton), Phase 2 (drift authority), Phase 5 (FM), Phase 6 (shell/panel):** all direct analogues of already-shipped LFO code/patterns (POD boundary, additive authority fields, exp-domain FM summation, shell/widget/display wiring) — HIGH confidence, well-documented in this repo's own source.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH (anti-aliasing technique choice, pitch, sync); MEDIUM (exact character↔BLEP tuning) | polyBLEP/BLAMP selection is DAFx-literature-grounded and cross-validated against Fundamental/Bogaudio/Surge/Plaits precedent; the character-interaction judgment call is explicitly flagged as a per-implementation tuning decision |
| Features | HIGH | VCV voltage/behavior conventions formally documented; reference modules (Fundamental, Bogaudio, EvenVCO, Surge, Plaits/Braids) verified against VCV Library + vendor docs; engine reuse verified by direct source read |
| Architecture | HIGH on integration/wiring (verified against actual source); MEDIUM on the morph-aware polyBLEP DSP design and alias-floor tolerances (standard technique, no bit-frozen reference exists yet in-repo) |
| Pitfalls | HIGH (code-grounded — every pitfall traces to the actual DSP headers, Makefile, or the v2.0.0 rejection post-mortem); MEDIUM on exact CPU figures and VCV Library update mechanics |

**Overall confidence:** HIGH — all four agents independently converged on the same crux (morph-aware BLEP), the same protective architecture (wrapper-not-edit, additive drift authority), and the same build order, which cross-validates the synthesis.

### Gaps to Address

- **Coarse/fine tune ranges:** STACK/FEATURES suggest COARSE ≈ ±5 octaves continuous (optional right-click snap) and FINE ≈ ±1 semitone, but these are recommendations, not requirements — confirm during requirements/roadmap review.
- **Drift-authority audition value:** the mechanism (separate `DriftEngine` instance, configurable authority) is settled, but the actual musical value (single-digit cents, per ARCHITECTURE's ~0.3–1.8% estimate) needs operator audition, not just calculation — flag for an audition-gated decision during the drift phase, matching the v1.4 x1.5/÷1.5-ratio precedent.
- **DC-blocker policy:** PITFALLS.md flags this as an explicit open decision (accept DC on the audio output, matching some real analog VCOs, vs. add a light DC blocker) — needs a deliberate call, not a default inheritance of the LFO's DC-positive stance.
- **Alias-floor test threshold:** target ≈ −60 dB rel. fundamental is a starting estimate (STACK/ARCHITECTURE), to be empirically refined once naive vs. band-limited renders exist to compare — pin during Phase 3.
- **VCV Library feature-update procedure:** the rejection-resubmission flow is documented from the v2.0.0 post-mortem, but publishing a *new module* to an already-live plugin's mechanics (auto-pickup from manifest version vs. requiring fresh action on #929) should be verified against current VCV Library docs before the resubmission phase.

## Sources

### Primary (HIGH confidence)
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) — 1V/oct, C4 baseline, ±5V output, saturation, polyphony guidance
- [VCV MinBlepGenerator API](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1MinBlepGenerator) / [minblep.hpp source](https://vcvrack.com/docs-v2/minblep_8hpp_source) — SDK-coupled precedent, rejected for the Rack-free core
- [DAFx-2016 "Rounding Corners with BLAMP" (Esqueda/Bilbao/Valimaki)](https://www.dafx.de/paper-archive/2016/dafxpapers/18-DAFx-16_paper_33-PN.pdf) — polyBLAMP for slope discontinuities
- [DAFx-2017 "Efficient Anti-aliasing of a Complex Polygonal Oscillator"](http://www.dafx17.eca.ed.ac.uk/papers/DAFx17_paper_100.pdf) — direct precedent for BLEP at analytically-known discontinuities of a morphed shape
- Repo source read directly: `src/dsp/Waveshape.hpp`, `LfoCore.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp`, `plugin.cpp`, `plugin.hpp`, `plugin.json`, `Makefile`, `tests/BlockDriver.hpp`, `tools/capture_golden.cpp`, `.planning/PROJECT.md`, `.planning/RETROSPECTIVE.md` — engine reuse, bit-identity landmines, v2.0.0 rejection post-mortem

### Secondary (MEDIUM confidence)
- [VCV Fundamental VCO-1/VCO-2 library page](https://library.vcvrack.com/Fundamental/VCO2), [Bogaudio VCO](https://library.vcvrack.com/Bogaudio/Bogaudio-VCO) / [BogaudioModules README](https://github.com/bogaudio/BogaudioModules) — reference oscillator anti-aliasing/feature comparisons
- [Befaco Even VCO](https://www.befaco.org/even-vco/) / [VCV Library](https://library.vcvrack.com/Befaco/EvenVCO) — mono-VCO precedent
- [Surge XT VCV Rack manual](https://surge-synthesizer.github.io/rack_xt_manual/), [Instruō Cš-L manual](https://www.instruomodular.com/wp-content/uploads/2019/09/Cs-L-Manual-A5.pdf) — morph/macro-oscillator genre framing
- [Martin Finke — PolyBLEP Oscillator](https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/), [metafunction — BLITs & BLEPs](https://www.metafunction.co.uk/post/all-about-digital-oscillators-part-2-blits-bleps), [KVR BLEP/minBLEP thread](https://www.kvraudio.com/forum/viewtopic.php?t=248390) — canonical polyBLEP form and tradeoffs

### Tertiary (LOW confidence)
- VCV Library feature-update publish mechanics for an already-live plugin — inferred from the rejection-resubmission flow, not yet verified against current docs (see Gaps to Address)

---
*Research completed: 2026-07-20*
*Ready for roadmap: yes*
