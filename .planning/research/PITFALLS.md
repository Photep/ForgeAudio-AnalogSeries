# Pitfalls Research

**Domain:** Adding an audio-rate, morph-aware, band-limited analog VCO to an existing C++11 / Rack-free / golden-pinned VCV Rack 2 plugin (second module, shipped LFO must not regress)
**Researched:** 2026-07-20
**Confidence:** HIGH (code-grounded; all claims trace to the four DSP headers, the Makefile, and the v2.0.0 rejection post-mortem). MEDIUM on exact CPU figures and the precise VCV Library *update* mechanics (flagged inline).

> This file is deliberately specific to THIS system. Generic "band-limit your oscillator" advice is omitted; every pitfall below is anchored to the reused `Waveshape`/`DriftEngine`/`RackCompat` core, the `-std=c++11` toolchain, the `-ffp-contract=off` golden fixtures, or the live VCV submission (#929).

---

## Critical Pitfalls

### Pitfall 1: Applying polyBLEP to a morph crossfade as if it had one discontinuity at the phase wrap

**What goes wrong:**
The morphed wave is a *continuous linear crossfade* across sine → tri → saw → square → pulse (`Waveshape::morphedWave`, `shapes[segment] + frac*(shapes[segment+1]-shapes[segment])`). Its discontinuities are NOT all at the phase wrap:
- saw: one step at phase 0/1 (wrap)
- square: steps at 0.5 **and** 1.0
- pulse: steps at `duty` (varying 0.50→0.05) **and** 1.0
- triangle: *slope* breaks (not value steps) at 0 and the valley (~0.5) — needs polyBLAMP, not polyBLEP
- saw "capacitor reset" and triangle "rounded peaks" (character) add *more* slope breaks

The naive "crossfade then correct" approach estimates a single jump `value(1⁻) − value(0⁺)` and BLEPs only the wrap. It silently misses the interior edges (square's 0.5 edge, pulse's `duty` edge, all the slope breaks). Those uncorrected edges alias, and — because their energy folds down from above Nyquist — the grit **only becomes audible at high notes**, which is exactly how teams ship this bug without noticing at C3.

**Why it happens:**
polyBLEP is usually taught on a single sawtooth. Developers reach for "one BLEP at the wrap" and forget the morphed signal is a *sum/blend* of primitives whose discontinuities live at different phases. Testing at mid pitch on a scope hides it.

**How to avoid:**
- Exploit BLEP linearity: `BLEP(a·f + b·g) = a·BLEP(f) + b·BLEP(g)`. For each sample step, enumerate every discontinuity the phase *crossed* (wrap, 0.5, `duty`, valley), compute each one's **effective signed jump/slope** as the morph-weighted (and bleed-adjusted, character-adjusted) combination of the contributing primitives, then apply one polyBLEP (steps) or polyBLAMP (slope breaks) per crossed discontinuity at its sub-sample position.
- Compute the jump from the **characterized** shape value, not the ideal ±2 step (see Pitfall 1a).
- Keep the BLEP/BLAMP machinery in NEW code (`VcoCore.hpp` / a new `BandLimited.hpp`) that *consumes* `Waveshape`; do not fold it into the frozen `Waveshape.hpp` (Pitfall 6).

**Warning signs:**
Sweep the morph knob while playing a high note (>~2 kHz fundamental): aliasing that appears/disappears as you enter the square/pulse region; spectrum shows inharmonic mirror partials folding down; THD-vs-pitch test rises sharply above a pitch threshold.

**Phase to address:** Morph-aware anti-aliasing phase (the core VCO band-limiting phase — its own phase, with a spectrum/THD harness).

---

### Pitfall 1a: The "character" softening fools the discontinuity-magnitude estimate (over- or under-correction)

**What goes wrong:**
`computeSquare`/`computePulse` apply tanh edge softening (`edgeWidth = c*0.08`, `sharpness = 1/edgeWidth`) and `computeSaw`/`computeTriangle` add a "capacitor reset" / "rounded peaks". At high character the edge is no longer a true ±2 step — it's a fast tanh ramp. If BLEP assumes the ideal jump height, it over-corrects (notch/overshoot ringing); if it ignores character entirely it can under-correct. Also note: the tanh softening is a **phase-domain** smoothing (fixed fraction of a cycle), so at high pitch it spans very few samples and does **not** band-limit — it is not a substitute for BLEP.

**Why it happens:**
The character deformation and the anti-aliasing are developed separately; the BLEP author assumes a canonical square/saw jump.

**How to avoid:**
Derive the effective jump for BLEP from the actual characterized shape at the discontinuity (evaluate the shape at phase just-before and just-after, or carry an analytic jump that includes the character factor `c`). Treat character-induced slope breaks (rounded peaks, capacitor reset) as BLAMP targets. Do not assume character softening buys any aliasing reduction.

**Warning signs:** Aliasing/ringing changes character (pun intended) as the character knob moves at fixed high pitch; a notch or overshoot at edges when character is high.

**Phase to address:** Morph-aware anti-aliasing phase (jointly with Pitfall 1).

---

### Pitfall 1b: The bleed term and normalization change the true discontinuity height

**What goes wrong:**
`morphedWave` adds neighbor-shape **bleed** (`result += bleedIntensity * bleedSignal`) then **normalizes** (`result /= (1 + bleedIntensity)`). The neighbors (leftIdx/rightIdx via the wrapping ring) have their own discontinuities, and the normalization scales the whole output. A BLEP magnitude computed from the pre-bleed, pre-normalization wave is wrong by the bleed contribution *and* the `1/(1+bleedIntensity)` factor. The bleed also modulates slowly via `bleedLfo` (drift OU-layer-0), so the correct jump is time-varying.

**Why it happens:**
Bleed and normalization are applied after the primary crossfade; a BLEP author naturally computes the jump from the "main" shape and forgets the post-processing.

**How to avoid:**
Compute the effective discontinuity **after** bleed and normalization: `effectiveJump = (crossfadeJump + bleedIntensity·neighborJump) / (1 + bleedIntensity)`. Because all operations are linear/affine per sample, the combined jump is a weighted sum you can form analytically alongside the value.

**Warning signs:** Residual aliasing that scales with the character knob (bleed is gated by `character >= 0.001`); BLEP correction slightly off only when bleed is active.

**Phase to address:** Morph-aware anti-aliasing phase.

---

### Pitfall 1c: The variable-width pulse — overlapping / skipped discontinuities at narrow duty

**What goes wrong:**
`pulseDuty` sweeps 0.50 → 0.05. As duty → 0.05, the rising edge (phase 0) and falling edge (phase `duty`) get very close. A polyBLEP kernel spans ±1 sample around a discontinuity; when the two edges are within ~2 samples of each other (narrow pulse at high pitch), the kernels **overlap** and must be summed. Worse, when `duty · period < 1 sample`, the phase increment can **step over the entire high region in one sample** — the pulse's up-and-back-down happens *between* samples and must be reconstructed as two discontinuities inside one step, or the pulse silently vanishes/aliases.

**Why it happens:**
Standard polyBLEP code assumes at most one discontinuity per sample and non-overlapping kernels — true for a saw, false for a narrow pulse at high frequency.

**How to avoid:**
Detect **all** discontinuities whose phase lies within `[phase, phase+deltaPhase)` each sample (there can be ≥2), place each BLEP at its own fractional position, and **sum** overlapping residuals. Guard the degenerate case `duty·period < 1 sample`.

**Warning signs:** Narrow-pulse end of the morph sweep sounds thin/aliased/absent at high notes; PWM-style timbres lose body at the top of the keyboard.

**Phase to address:** Morph-aware anti-aliasing phase (explicit narrow-pulse test case).

---

### Pitfall 2: Reusing the LFO-grade `exp2_taylor5` for VCO pitch — the real risk is NOT the poly error

**What goes wrong (and what doesn't):**
`forge::exp2_taylor5` computes the octave (integer) part **exactly** via the IEEE exponent field (`exp2Floor`: `xi<<23`) and only the fractional part through the degree-5 minimax poly. Therefore pitch does **NOT** drift sharp/flat over many octaves — each octave is exact by construction; only the within-octave interpolation carries the poly error (order ~1e-4 relative ≈ ~0.1 cent, non-cumulative, inaudible). **`exp2_taylor5` is more than accurate enough for a VCO.** Do not waste a phase "upgrading" it.

The actual pitfalls in the pitch path are:
1. **Exponent overflow / wide FM:** `exp2Floor` does `x += 127; int32 xi = (int)x; yii = xi<<23`. Truncation toward zero equals floor only while `x+127 ≥ 0` (true for all audio pitches). But a large **audio-rate FM** excursion (or future through-zero FM) can push the exponent past the float range → `xi<<23` overflow → NaN/Inf → silence or a stuck voice. **Clamp the summed pitch CV before the exp.**
2. **Float phase precision at the BLEP sub-sample crossing:** BLEP quality depends on the fractional sample position of each discontinuity (`frac = overshoot / deltaPhase`). A float32 phase near 1.0 quantizes to ~6e-8 steps; the resulting jitter in the crossing position becomes a BLEP placement error → a residual noise floor. **Accumulate phase in `double`** (at minimum compute the crossing fraction in double), even though the LFO used float.
3. **Wrong tuning reference:** the LFO maps a Hz knob; a VCV VCO must use the standard `C4 = 0 V → 261.6256 Hz` (`dsp::FREQ_C4`) reference, `freq = FREQ_C4 · exp2(pitch)`. Reusing the LFO's base-frequency mapping tunes the VCO to the wrong reference vs every other module.
4. **Double-exponentiation / multiplicative FM:** sum all pitch contributions (V/Oct + coarse + fine + FM CV) in the **exponent domain**, then one `exp2`. Multiplying frequencies or calling exp twice yields linear-FM behavior or detuning.

**Why it happens:**
"It's only a 5th-order approx, surely it drifts over 10 octaves" — a plausible but wrong worry; meanwhile the genuine failure modes (overflow, float crossing jitter, wrong C4) get overlooked.

**How to avoid:**
Clamp pitch pre-exp; use `double` phase (or double crossing math); pin the C4 reference and assert it in a test; sum in the exp domain with a single `exp2_taylor5` call reused verbatim (keep it bit-identical — it's on the golden path).

**Warning signs:** A tuner shows a *fixed* cents offset vs Fundamental VCO (wrong C4) — distinct from a *growing* offset per octave (accumulation/linear-FM bug) — distinct from dropouts at extreme registers (overflow/clamp). Noise floor rises with pitch even drift-off (float crossing jitter).

**Phase to address:** VCO pitch / V-Oct phase (early, before anti-aliasing). Extend the existing `BlockDriver` frequency-accuracy invariant to sweep the full MIDI range and assert < 1 cent error.

---

### Pitfall 3: Hard sync via naive phase reset (click + aliasing), and mis-reusing the LFO's 3 ms anti-click crossfade

**What goes wrong:**
A hard reset of the slave phase mid-cycle steps the output from its current value to the value at the reset phase — a discontinuity → click + aliasing on every master cycle. Two specific traps here:
- **Placing the sync BLEP at the slave's phase** instead of the **master's** wrap fraction. The sub-sample sync instant is determined by where the *master* crossed its wrap within the sample, not the slave.
- **Reusing the LFO's 3 ms cosine anti-click crossfade** (used for CLK/RESET phase resets). At audio rate 3 ms ≈ 130 samples — many cycles of a high oscillator. Applied to hard sync (which fires *every master cycle*), it smears/mutes the sync and destroys the buzzy sync timbre that depends on the sharp reset. The cosine crossfade is correct for *occasional* LFO-rate resets; hard sync needs a **sync-BLEP**, not a crossfade.

**Why it happens:**
The codebase already has a working "click-free reset" primitive (the cosine crossfade), so it's tempting to route hard sync through the same `processReset()` path. It's the wrong tool at audio rate and per-cycle rate.

**How to avoid:**
- Dedicated sync path: on master wrap, compute the reset jump `slaveValue(now) − slaveValue(resetPhase)` (including character/morph/bleed, same combined-discontinuity machinery as Pitfall 1) and apply a polyBLEP at the **master's** sub-sample wrap fraction. Share the BLEP residual accumulator with the free-run anti-aliasing.
- Keep hard sync **off** the cosine-crossfade `processReset()` path entirely.
- Handle ≥1 sync event per sample (master faster than slave) and the case where the reset itself makes the slave cross its own discontinuity in the same step.

**Warning signs:** Audible click per sync; sync sounds dull/soft (crossfade smear) instead of buzzy; aliasing on the classic sync sweep; DC thump on reset.

**Phase to address:** Hard-sync phase (after the BLEP core exists; depends on Pitfall 1's combined-discontinuity infrastructure).

---

### Pitfall 4: Drift authority tuned for sub-audio detunes a VCO by a semitone

**What goes wrong:**
`DriftEngine::step` uses `maxDrift = 0.075` (free) — **7.5% frequency deviation**. On a 2 Hz LFO that's a slow musical wobble. On an audio VCO, ±7.5% ≈ **±125 cents** — audibly out of tune / broken. The jitter and DC-offset authorities are likewise LFO-scaled. Two nuances:
- The OU *statistics* are already sample-rate-independent: `sigma·sqrtDt·noise` is the correct SDE discretization, so the stationary variance is constant across 44.1/48/96 kHz — drift will **not** blow up with sample rate. The problem is purely the **authority multipliers** being audio-inappropriate.
- The **`dcOffsetV`** output is an LFO modulation/visual feature (DC wander of a control voltage). Summed into an *audio* signal it's just DC — usually unwanted on a VCO output (and it compounds the pulse's intrinsic DC, Pitfall 4a). Consider dropping/zeroing `dcOffsetV` for the VCO.

**Why it happens:**
The engine is reused verbatim; its constants read as "the drift amounts," so it's easy to inherit them wholesale at audio rate.

**How to avoid:**
Introduce VCO-appropriate authority (single-digit cents at full drift). **Do not hard-code new constants by editing `DriftEngine::step`'s draw sequence** (Pitfall 4b) — instead pass authority as a parameter with a default that preserves the LFO's exact values, or scale the returned `deltaPhaseMul`/`dcOffsetV` externally in `VcoCore`. Decide whether the VCO uses `dcOffsetV` at all.

**Warning signs:** VCO sounds warbly/detuned even at low drift; the drift knob makes it unusable well before max; audible DC thump / offset on the output.

**Phase to address:** Audio-rate analog-engine phase.

---

### Pitfall 4a: Pulse/asymmetric-shape DC on an audio output

**What goes wrong:**
A narrow pulse (duty 0.05) and character-induced duty asymmetry produce large intrinsic DC. The LFO *wants* DC (it's a modulation source); an audio VCO generally does not. Combined with drift `dcOffsetV` (Pitfall 4) this can bias the signal, waste headroom, and thump downstream envelopes/filters.

**How to avoid:** Decide the DC policy explicitly — either accept it (many analog VCOs pass DC) or add a light DC blocker on the audio output. If added, capture it in the golden and account for its `-ffp-contract`/portability behavior (it's a one-pole; the `OnePole` snap idiom in `RackCompat.hpp` is the reference). Do not silently inherit the LFO's DC-positive stance.

**Phase to address:** Audio-rate analog-engine phase / VCO output-stage decision.

---

### Pitfall 4b: Perturbing the shared RNG draw order and breaking the LFO's goldens

**What goes wrong:**
`DriftEngine.hpp`'s contract is load-bearing: **exactly 4× OU + 1× jitter + 1× DC** `normalDist(rng)` draws per sample, in that order (the header banner says so). The LFO's golden `.f32` fixtures replay this exact stream. If the VCO shares one `DriftEngine` instance with the LFO, or edits `step()` to add/remove/reorder a draw (e.g. a 7th draw for a VCO-only parameter, or drift decimation inside `step()`), the RNG stream shifts and **every drift-on LFO golden breaks** — and worse, could ship a silent LFO behavior change.

**Why it happens:**
Adding a VCO-specific drift feature "just needs one more random number" — dropped into `step()` where it's convenient.

**How to avoid:**
- The VCO owns a **separate `DriftEngine` instance** with its own `rng`/seed — the LFO's engine is then physically untouched.
- Treat `DriftEngine::step`'s draw sequence as **frozen**. Any new authority is a *parameter*, not a new draw. Any structural change must re-run the full LFO golden suite as a regression before merge (the draw order is the canary).

**Warning signs:** LFO golden replay fails after any `DriftEngine.hpp` touch; drift-on regression diffs.

**Phase to address:** Audio-rate analog-engine phase; enforced by the LFO-golden regression running in CI on every commit.

---

### Pitfall 5: Reintroducing the exact v2.0.0 rejection — ODR'd static constexpr & C++17-isms the toolchain rejects but clang masks

**What goes wrong:**
The VCV library builds **every** platform with `-std=c++11` (GCC on win/linux). Local mac clang at `-O3` folds/masks two whole bug classes that got v2.0.0 rejected:
1. In-class `static constexpr` arrays that are **runtime-indexed** are declarations only under C++11 — ODR-use (taking the address via indexing) needs an **out-of-line definition**, or MinGW `ld` fails with `undefined reference` (this is the exact win-x64 failure that rejected v2.0.0).
2. C++17-isms: `inline constexpr` variables, `[[maybe_unused]]`, `if constexpr`, structured bindings, nested-namespace `a::b {}`, and **`std::clamp`** (C++17 — this is why `RackCompat.hpp` ships its own `forge::clamp`).

A brand-new `AnalogVCO.cpp` module TU is a **fresh opportunity to reintroduce the ODR class** (VCO modules love tables: waveform-name arrays, coarse/semitone tables, sync-mode labels). Any in-class `static constexpr` array indexed at runtime re-triggers the rejection.

**Why it happens:**
The DSP tests compile `-std=c++17` and never link the Rack shell, so `make test` and CI test legs **do not catch** C++11/ODR issues in the module TU. Local clang builds green. The bug only surfaces at MinGW **link** time.

**How to avoid:**
- Prefer **namespace-scope `static constexpr`** in headers (the `forge::` pattern, e.g. `MathConst.hpp`) which sidesteps ODR entirely; or provide an explicit **out-of-line definition block** after the module struct (the `AnalogLFO.cpp` "Out-of-line definitions" precedent).
- Run `make strict` (`-std=c++11 -pedantic-errors -fsyntax-only`) on every commit — it globs `src/*.cpp`, so `AnalogVCO.cpp` is covered automatically. **But note `make strict` is `-fsyntax-only`: it does NOT catch the ODR *link* failure** — the CI MinGW **link** leg is the only gate that does. Do not tag/submit on a green `make strict` alone.
- No `std::clamp`, no `inline constexpr`, no `[[maybe_unused]]`, no `if constexpr` in shipped `src/` code.

**Warning signs:** `make strict` errors (C++17-ism); MinGW CI link leg `undefined reference` (ODR); green locally + green tests but red MinGW.

**Phase to address:** Every VCO phase (discipline), gated by the VCO test-harness phase wiring the MinGW link leg + `make strict` for the new TU.

---

### Pitfall 5a: New VCO goldens that are silently non-portable (fma contraction, `normal_distribution`, transcendental ULPs)

**What goes wrong:**
The LFO's **drift-OFF** goldens are cross-platform bit-exact (3-OS CI), but its **drift-ON** goldens had to be **macOS-gated** because `std::normal_distribution` differs between libstdc++ (win/linux) and libc++ (mac). The VCO inherits this and adds new bit-stability hazards:
- **FMA contraction:** `a*b+c` fuses differently per platform. polyBLEP polynomials, phase accumulation, crossfades, and the OU step are all `a*b+c`. The Makefile pins `-ffp-contract=off` for `test`/`capture`; new VCO DSP must be captured/replayed under the same flag, and the drift path stays `normal_distribution`-bound.
- **Transcendental ULPs:** `std::sin/exp/tanh/cos` can differ in the last ULP across libm. The waveshape already uses all four and its drift-off goldens happen to agree across the CI toolchains — but **any new transcendental call** in the BLEP/sync/output path is a fresh chance to introduce a platform divergence the LFO never had.
- **Float literals / denormals:** mixed `2.f`/`2.0` promotion (the `kPi` constant is deliberately IEEE-identical — follow that discipline for a new `FREQ_C4`); denormal OU/filter states flush-to-zero differently per platform (also a CPU trap, Pitfall 7a) and break bit-identity.

**How to avoid:**
Decide the fixture policy **up front** (mirror v1.4 Phase 22): **drift-OFF VCO goldens are portable** (deterministic, no `normal_distribution`) and gate all 3 OS; **drift-ON VCO goldens are single-platform-gated** like the LFO's. Capture with `-ffp-contract=off`. Never enable `-ffast-math`. If a new transcendental causes a 3-OS diff, platform-gate that specific fixture (the established pattern) rather than chasing bit-parity.

**Warning signs:** A golden that passes on mac and fails on linux/win CI (or vice-versa); a diff that appears only when drift is on; a diff that appears only after adding a `tanh`/`sin` call.

**Phase to address:** VCO test-harness phase (built FIRST, before the DSP — test-harness-before-refactor, the v1.4 lesson).

---

### Pitfall 6: Regressing the shipped LFO via shared-header edits

**What goes wrong:**
`Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp` are **shared** by the live LFO. The morph-aware BLEP work will tempt edits to `Waveshape.hpp` (e.g. to expose per-shape discontinuity descriptors). **Any change to existing math in those headers changes LFO output and breaks its goldens** — and could ship a silent regression to a live module.

**Why it happens:**
The discontinuity info the VCO needs (jump heights, edge phases) lives conceptually "inside" `Waveshape`, so it feels natural to add it there and reach into existing computations.

**How to avoid:**
- Treat the four shared headers as **frozen**. Put VCO band-limiting in **new** files (`VcoCore.hpp`, `BandLimited.hpp`).
- If you *must* extend a shared header, make it **purely additive** — a new `const` method that touches no existing member or computation (e.g. `float discontinuityAt(phase) const`) — and prove byte-identity by re-running the full LFO golden suite. The `bleedLfo` D-05 lift already gives the VCO what it needs (`morphedWave(..., bleedLfo)`), so the VCO can drive `Waveshape` with **no edit at all**.
- Keep the "ZERO Rack-SDK includes" hygiene in any new header (so it links into `make test`).

**Warning signs:** LFO golden replay fails after touching a shared header (the canary); any diff in a shared header's existing lines in code review.

**Phase to address:** Every VCO phase (freeze discipline from phase one); LFO golden regression in CI.

---

### Pitfall 6a: Multi-module manifest / slug / versioning & VCV update mechanics

**What goes wrong:**
Adding a second module to an already-**accepted, live** plugin (#929) is an **update**, not a new submission. Failure modes:
- **Slug immutability:** the new module needs a unique, **permanent** slug (e.g. `AnalogVCO`). Once users have patches referencing it, it can never change — choose carefully. The plugin slug `ForgeAudio-AnalogSeries` and the LFO slug `AnalogLFO` are fixed forever.
- **Forgetting `addModel`:** a second `addModel(modelAnalogVCO)` in `plugin.cpp` `init()` + a second `modules[]` entry in `plugin.json`, or the module never appears in Rack.
- **Version:** the manifest `version` stays **Rack-major** (`2.x.x`); it was bumped to `2.0.1` at the rejection resubmission, so the VCO release continues that line (e.g. `2.1.0`). Internal GSD label "v2.0" ≠ manifest version — the v1.4→tag-v2.0.0 mismatch already bit once; reconcile deliberately.
- **Never move an already-referenced tag** — cut a new tag for the release.
- **A VCO build failure now blocks shipping ANY update to the whole plugin**, including LFO fixes — so the VCO must pass the strict/MinGW gate before any tag.

**Update-mechanics uncertainty (MEDIUM confidence — verify before shipping):** the retrospective documents the *rejection-resubmission* flow (fix → bump version + new tag → comment on the toolchain-filed issue in our repo → the reporter/team closes when live). The precise flow for publishing a *feature update* (new module) to an already-live plugin — whether the library auto-picks the new tag from the manifest version or requires a fresh action on #929 — should be confirmed against current VCV Library docs at release time.

**How to avoid:**
Pin the new slug early and treat it as permanent; wire `addModel` + `modules[]` in the scaffold phase; bump manifest version on the 2.x line with a fresh tag; strict-gate the VCO before tagging; confirm the current library update procedure before publishing.

**Warning signs:** Module absent in Rack (unregistered); library update rejected (version/tag/slug); a rename request after users have patches (too late).

**Phase to address:** Integration & resubmission phase (final), with slug/`addModel` set in the VCO scaffold phase.

---

### Pitfall 6b: Copying the LFO's hostile-input & seqlock bugs into the VCO

**What goes wrong:**
The VCO reuses `Xoroshiro128Plus` + `std::normal_distribution` and will serialize a seed (component spread / drift). The **all-zero seed** is a Xoroshiro fixed point → `normal_distribution` infinite loop → **Rack hangs on patch load** (a v2.0.0 latent bug fixed in the LFO). Likewise, if the VCO copies the LFO's display **seqlock** reader, copying the *pre-fix* idiom (`continue` inside `do/while` jumps to the condition → validates a torn/never-copied snapshot) reintroduces that bug.

**How to avoid:**
Validate any deserialized seed (reject/replace `{0,0}` and any all-zero pair) exactly as the LFO fix does — parse success ≠ valid domain. Copy only the **corrected** seqlock reader idiom (spin-to-even as an *inner* loop; writer: odd store → release fence → payload → release even store). Reuse the LFO's fixed patterns, not their historical versions.

**Warning signs:** Rack hangs loading a hand-edited/old patch; display shows torn/garbage frames under load.

**Phase to address:** VCO state/serialization phase and display phase.

---

### Pitfall 7: CPU — a full-audio-rate VCO is nothing like the cheap sub-audio LFO

**What goes wrong:**
`morphedWave` computes **all** of sine (`std::sin`), triangle (`fabs`), saw (`std::exp`+`cos`), square (`tanh`), and pulse (`tanh`) **every sample**, plus bleed (re-reads neighbor shapes), plus 6× `std::normal_distribution` draws (each Box-Muller = `log`/`sqrt`/`cos` internally), plus `exp2_taylor5` for pitch, plus polyBLEP. At sub-audio the LFO pays this rarely; at audio rate it's per-sample. Stacking many instances can dropout.

**Why it happens:**
The reused core was written for perceptual richness at LFO rates where CPU was a non-issue; the cost is invisible until it runs 48,000×/sec.

**How to avoid:**
- **Compute only contributing shapes.** The crossfade touches 2 shapes; bleed's wrapping ring touches up to 2 neighbors → at most 4 of 5. Skipping the untouched shape(s) is a legitimate win **but must be bit-verified against a reference** (it changes control flow, not math — guard with a golden).
- **Consider drift decimation** (update OU at a control-rate, hold between): valid for the VCO because it owns its own goldens — but this **changes the draw count/order**, so it must NOT touch the shared `DriftEngine::step` used by the LFO (Pitfall 4b); implement as a VCO-side wrapper or a separate stepping cadence with its own captured goldens.
- **No oversampling in v2.0** (deferred to v2.1) keeps CPU at 1× — but design headroom: when 2×/4× lands, shapes + BLEP + drift all multiply.
- Profile early with several instances; target a small single-digit % CPU per voice.

**Warning signs:** Rack CPU meter high with a few instances; dropouts; users report load.

**Phase to address:** VCO core DSP phase (design the compute-only-active-shapes / decimation decisions there to avoid rework) + a dedicated profiling pass.

---

### Pitfall 7a: Denormals at audio rate (CPU spikes + bit-instability)

**What goes wrong:**
OU states, filter states, and decaying BLEP residuals can go **denormal**, causing large CPU spikes on some x86 platforms and platform-dependent flush-to-zero that also breaks bit-identity (Pitfall 5a).

**How to avoid:**
Add tiny denormal prevention (e.g. a DC-offset dither or flush below a threshold) **consciously** — but note it changes bits, so capture goldens with it in place and verify portability. Prefer a bit-stable approach over relying on per-platform FTZ/DAZ.

**Warning signs:** Sporadic CPU spikes when the oscillator is idle/decaying; a golden that diverges only after long-running silence.

**Phase to address:** VCO core DSP / performance pass.

---

### Pitfall 8: Carrying over the CRT display by capturing live audio samples

**What goes wrong:**
The LFO's single-cycle display buffer assumes ~256 points per cycle. At audio rate a cycle can be **far shorter than 256 samples** (e.g. 48 samples at 1 kHz / 48 kHz). Capturing live audio samples into the display buffer will alias/garble the trace at high pitch.

**Why it happens:**
The display "just worked" at LFO rates where a cycle spans thousands of samples; the assumption is invisible until audio rate.

**How to avoid:**
Carry over the LFO's **off-thread regeneration** approach: snapshot the shape parameters (morph/character/drift/duty) via the seqlock and **regenerate** a single normalized cycle from `Waveshape` off the audio thread for display — do **not** capture live audio samples. This decouples the display from pitch. (And copy the corrected seqlock idiom — Pitfall 6b.)

**Warning signs:** Display trace turns to noise/aliases as pitch rises; frame stutter from audio-thread buffer fills.

**Phase to address:** VCO display phase.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Single BLEP at the phase wrap only | Fast to write, sounds fine at mid pitch | Interior square/pulse edges alias at high notes; hard to retrofit | **Never** — the whole value prop is a clean morph across the keyboard |
| Reuse LFO drift authority (7.5%) at audio rate | Zero new tuning | ±125-cent detune, unusable drift knob | Never ship; only as a scaffolding placeholder behind a TODO |
| Reuse the 3 ms cosine crossfade for hard sync | Existing click-free primitive | Smeared/muted sync, wrong timbre | Never for hard sync; fine for CLK/RESET-style resets |
| Edit `Waveshape.hpp`/`DriftEngine.hpp` to expose VCO internals | Convenient access to jumps/draws | Breaks live-LFO goldens; risks silent LFO regression | Only if **purely additive** + full LFO golden re-verify |
| Skip drift decimation (per-sample OU at full rate) | Simpler, matches LFO exactly | Higher CPU; 6 Box-Muller draws/sample | Acceptable for v2.0 if CPU budget holds; revisit under oversampling |
| Tag/submit on green `make strict` alone | Faster release | `make strict` is `-fsyntax-only` — misses ODR link failure (the v2.0.0 class) | Never — require the MinGW **link** leg green |
| Ship drift-ON VCO goldens as 3-OS bit-exact | "Full" cross-platform coverage | `normal_distribution` diverges → CI flaps | Never — gate drift-ON goldens to one platform (LFO precedent) |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| VCV Library update (#929, live) | Treating a new module as a new submission; moving an existing tag | Bump manifest `version` on the 2.x line, cut a **new** tag, follow the current library update flow (verify docs); slug is permanent |
| `plugin.cpp` / `plugin.json` | Forgetting the 2nd `addModel` / `modules[]` entry | Register `modelAnalogVCO` + add the manifest module entry in the scaffold phase |
| Shared DSP headers | Reaching into `Waveshape`/`DriftEngine` internals for the VCO | Drive them via existing params (`morphedWave(..., bleedLfo)`, separate `DriftEngine` instance); keep them frozen |
| `dsp::FREQ_C4` tuning reference | Reusing the LFO's Hz-knob base for the VCO | Standard `C4=0V→261.6256 Hz`, `freq = FREQ_C4·exp2(pitch)`; assert in a test |
| `make strict` vs CI MinGW link | Assuming syntax gate catches ODR | Keep the MinGW **link** leg as the ODR gate; strict is syntax-only |
| Patch JSON (seed) | Trusting parsed seed values | Reject all-zero/`{0,0}` seeds (Xoroshiro fixed point → hang) |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Compute all 5 shapes every sample | High CPU per voice | Compute only the ≤4 shapes the crossfade+bleed touch (golden-verified) | Noticeable with multiple instances / high sample rate |
| 6× `normal_distribution` (Box-Muller) per sample | CPU per voice | Consider VCO-side drift decimation (own goldens; don't touch shared `step`) | Multiple instances; worsens under future oversampling |
| Denormal OU/filter/BLEP-residual states | Sporadic CPU spikes when idle/decaying | Conscious, bit-stable denormal prevention | Long sustained/idle voices on some x86 |
| Deferred oversampling arrives (v2.1) | 2×/4× multiplies shapes+BLEP+drift | Design 1× path with headroom; keep shape/drift cost low now | v2.1 timbral/TZFM work |

## Security Mistakes

*(Not a networked/multi-user domain — the only adversarial surface is persisted patch state.)*

| Mistake | Risk | Prevention |
|---------|------|------------|
| Trusting deserialized RNG seed | All-zero seed → `normal_distribution` infinite loop → Rack hang on load (DoS-by-patch) | Domain-validate seed on load; reject/replace `{0,0}` and all-zero pairs |
| Unvalidated display-cache sentinels | OOB read / `(int)round(inf)` UB during first-block acquisition (the BPM-pill class) | Check `{-1,0}`-style sentinels at **every** draw gate, not just the original ones |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Drift knob detunes by a semitone at audio rate | VCO feels broken/out of tune | Re-scale drift authority to a few cents; keep the alive character subtle |
| Audio DC from narrow pulse + drift `dcOffsetV` | Headroom loss, downstream thump | Decide DC policy; likely drop `dcOffsetV` on audio out, optional DC blocker |
| Display aliases at high pitch | Trace looks like noise, undermines the "see the shape" value | Regenerate the single cycle off-thread from parameters, not live samples |
| VCO tuned to a non-standard reference | Won't play in tune with other modules | Pin `FREQ_C4` reference; verify against Fundamental VCO |

## "Looks Done But Isn't" Checklist

- [ ] **Anti-aliasing:** sounds clean at C3 but **test the top two octaves** — interior square/pulse edges and slope breaks are the usual miss (Pitfall 1/1a/1c).
- [ ] **V/Oct:** tracks one octave but **sweep the full MIDI range** and assert < 1 cent; check extreme-register overflow and float crossing-jitter noise floor (Pitfall 2).
- [ ] **Hard sync:** resets without a click on a scope but **listen** for smear (crossfade) vs buzz (BLEP), and confirm sync BLEP uses the **master's** fraction (Pitfall 3).
- [ ] **Drift:** subtle at low settings but **check max drift in cents** and confirm the LFO's drift-on goldens still pass (shared-engine canary — Pitfall 4/4b).
- [ ] **C++11 gate:** green locally + green tests but **MinGW link leg** green is the real ODR gate; `make strict` is syntax-only (Pitfall 5).
- [ ] **Goldens:** captured on mac but **3-OS CI** green for drift-off, and drift-on fixtures **platform-gated** (Pitfall 5a).
- [ ] **LFO regression:** VCO builds but **re-run the full LFO golden suite** — any shared-header touch is a regression risk (Pitfall 6).
- [ ] **Manifest:** module works locally but **`addModel` + `modules[]` entry + permanent slug + 2.x version bump + new tag** all present (Pitfall 6a).
- [ ] **Patch load:** loads a fresh instance but **load a hand-edited all-zero-seed patch** without hanging Rack (Pitfall 6b).
- [ ] **Display:** looks right at 100 Hz but **check at 2–5 kHz** — off-thread regeneration, not live capture (Pitfall 8).
- [ ] **CPU:** fine as one instance but **stack 8–16** and watch the meter / dropouts (Pitfall 7).

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Interior-edge aliasing shipped | MEDIUM | Refactor BLEP to per-crossed-discontinuity with combined jump; add high-pitch THD test; re-tag |
| Drift authority too high | LOW | Parameterize/scale authority externally; re-capture VCO drift goldens (LFO untouched) |
| Broke LFO goldens via shared header | MEDIUM | Revert the shared-header edit; re-implement additively; re-run LFO suite; the git history + goldens make this detectable immediately |
| ODR rejection on resubmit | LOW–MEDIUM | Move constants to namespace-scope or add out-of-line defs (known fix); bump version + new tag; comment on the toolchain issue (v2.0.0→2.0.1 playbook) |
| Non-portable golden flapping CI | LOW | Platform-gate the offending fixture (macOS-gate, LFO precedent); keep drift-off portable |
| All-zero-seed hang | LOW | Add seed domain validation on load (patch already exists from the LFO fix) |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| 1 / 1a / 1b / 1c — morph-aware BLEP/BLAMP | Morph-aware anti-aliasing phase (dedicated) | Spectrum/THD-vs-pitch harness; sweep morph at top octaves; narrow-pulse case |
| 2 — V/Oct & exp path | VCO pitch / V-Oct phase (early) | Full-MIDI-range frequency accuracy < 1 cent; overflow clamp test; C4 vs Fundamental |
| 3 — hard sync BLEP | Hard-sync phase (after BLEP core) | Sync sweep listen + spectrum; assert BLEP uses master fraction; no crossfade path |
| 4 / 4a / 4b — drift at audio rate | Audio-rate analog-engine phase | Max-drift-in-cents check; DC policy decision; **LFO golden regression** (canary) |
| 5 / 5a — C++11/ODR & bit-portable goldens | VCO test-harness phase (built FIRST) + every phase | `make strict` + **MinGW link leg**; 3-OS drift-off goldens; drift-on gated |
| 6 / 6b — protect the LFO; copy fixed idioms | Every VCO phase (freeze discipline) | LFO golden suite in CI on every commit; code-review shared-header diffs |
| 6a — manifest/slug/version/resubmit | Integration & resubmission phase (slug/`addModel` in scaffold) | Module appears in Rack; strict+MinGW green before tag; verify current library update flow |
| 7 / 7a — CPU & denormals | VCO core DSP phase + profiling pass | Multi-instance CPU meter; idle-voice spike check; golden-verified shape-skip |
| 8 — display at audio rate | VCO display phase | Trace correct at 2–5 kHz; off-thread regeneration; corrected seqlock |

## Sources

- `src/dsp/RackCompat.hpp` — `exp2_taylor5`/`exp2Floor`, `Xoroshiro128Plus`, `OnePole` snap idiom, `forge::clamp` (no `std::clamp`) [HIGH]
- `src/dsp/DriftEngine.hpp` — RNG draw-order contract (4 OU + 1 jitter + 1 DC), authority constants (0.075/0.02), sqrtDt SDE scaling, drift-low skip [HIGH]
- `src/dsp/Waveshape.hpp` — morph crossfade, character softening, variable-width pulse, bleed + normalization, `bleedLfo` D-05 lift [HIGH]
- `src/dsp/MathConst.hpp` — IEEE-identical `kPi`, "plain constexpr not `inline constexpr` (C++17)" discipline [HIGH]
- `Makefile` — `make strict` (`-std=c++11 -pedantic-errors -fsyntax-only`), `-ffp-contract=off` test/capture flags, Rack-free test target [HIGH]
- `.planning/RETROSPECTIVE.md` — v2.0.0 rejection post-mortem (ODR static constexpr, C++17-isms, clang masking, MinGW-link-first, all-zero-seed hang, seqlock `continue` bug, display-cache sentinels), drift-on macOS-gated goldens, resubmission mechanics [HIGH]
- `.planning/PROJECT.md` — v2.0 lean scope (oversampling/TZFM/phase-distortion deferred), same-plugin/second-module structure, slug/#929 constraints [HIGH]
- Band-limited synthesis theory (polyBLEP/polyBLAMP linearity, sync-BLEP master-fraction placement, minimax-exp octave-exactness) — established DSP literature [HIGH]
- VCV Library *feature-update* publish flow for an already-live plugin — [MEDIUM; verify against current library docs at release time]

---
*Pitfalls research for: audio-rate morph-aware analog VCO added to a C++11 / golden-pinned VCV Rack plugin*
*Researched: 2026-07-20*
