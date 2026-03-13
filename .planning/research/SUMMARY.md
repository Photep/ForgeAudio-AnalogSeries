# Project Research Summary

**Project:** Forge Audio Analog Series — v1.2 Deep Analog
**Domain:** VCV Rack Eurorack LFO module — analog character deepening, modulation inputs, groove features
**Researched:** 2026-03-13
**Confidence:** HIGH

## Executive Summary

v1.2 "Deep Analog" is a targeted feature expansion to an existing, well-tested 890-line single-file VCV Rack LFO module. The foundation — four-layer Ornstein-Uhlenbeck drift engine, three-knob analog engine (Morph, Character, Drift), clock sync, real-time waveform display — is solid and remains architecturally unchanged. All new features are additive: new enum members, new computations at defined insertion points in the existing `process()` pipeline, and one modified function signature (`computeMorphedWave()` gains a bleed parameter). No new dependencies are introduced. No new source files. No Makefile changes. The estimated code growth is ~200–250 lines, reaching roughly 1,100 lines total — still manageable as a single file.

The recommended build order is driven by data-flow dependencies and interaction risk management: display polish first (zero DSP risk), RESET and phase offset together (tightly coupled reset-target semantics), FM third (upstream frequency modifier), the four analog imperfections as a batch (all share the drift-scaling pattern), waveform bleed fifth (modifies the core waveform function signature), swing last (most complex clock interactions), and panel layout deferred until all DSP is finalized — a direct lesson from the v1.0 retrospective. After v1.2, the Forge Analog LFO will have a more comprehensive analog-character feature set than any other LFO in the VCV Rack ecosystem: the only module combining four-shape continuous morph, per-shape analog character modeling, multi-layer OU drift, clock sync with 15 ratios, FM input, phase offset, swing, expanded imperfections, and real-time waveform display.

The primary risks are feature interaction bugs, not algorithmic complexity. FM fighting clock-derived frequency, the 1ms RESET/CLK blanking requirement, phase offset affecting what "reset" means, and phase jitter accumulating against clock sync are four critical interaction patterns that must be tested together, not in isolation. The most important architectural constraint for the entire milestone is the distinction between accumulator modification and readout modification: phase offset, jitter, and component spread all affect waveform output but must apply to a local copy of phase — never to the accumulator itself, which is the single source of truth for cycle timing, beat counting, and swing boundary detection.

---

## Key Findings

### Recommended Stack

The v1.2 stack is the v1.0/v1.1 stack. Every new algorithm is hand-rolled using only C++17 standard library math and existing VCV Rack SDK primitives. No external DSP libraries are needed or appropriate for five-to-fifteen-line per-feature computations.

**Core technologies:**
- `std::pow(2.f, x)` (C++17 `<cmath>`): exponential FM conversion — correct over `dsp::exp2_taylor5()` for monophonic accuracy; one call per sample at LFO rates is negligible CPU cost
- `rack::dsp::SchmittTrigger` (second instance): independent RESET jack edge detection — same SDK pattern already used for CLK; add one member, no new architecture
- `rack::dsp::TExponentialFilter` (existing instance): pitch slew — make the existing `freqSlew` lambda drift-dependent rather than adding a second filter; avoids double-slew stacking
- NanoVG `nvgBoxGradient()` + `nvgTextBounds()`: HUD text backgrounds — soft-edged gradient halo (not a hard rectangle, which produced the "ugly black box" in v1.1 display testing)
- `Xoroshiro128Plus` RNG (existing): per-instance component spread offsets — generate once in constructor, serialize RNG seed (not spread values) for reproducibility across patch saves
- `std::normal_distribution` (existing): phase jitter noise draws — already used for OU layers; apply to waveform lookup copy only

**No new dependencies.** No new source files. The module stays single-file through v1.2. WaveformDisplay refactoring into a separate file is a v2.0 consideration at ~1,200+ lines.

**Serialization change:** The v1.0 decision of "no OU state serialization" stands for drift layers. New exception: the RNG seed for component spread IS serialized via `dataToJson`/`dataFromJson`, because component spread represents fixed hardware characteristics (like physical component tolerances), not random runtime behavior. Store the seed, not the spread values, so the generation algorithm can evolve without breaking saved patches.

**CPU budget:** FM adds one `std::pow(2.f, x)` per sample (~15–20 cycles, the most expensive single addition). All other features are cheaper than one OU layer update. Total estimated overhead: ~5–10% over v1.1. Well within budget for a monophonic LFO.

### Expected Features

**Must have (table stakes):**
- FM input jack — standard on every serious LFO/VCO in VCV Rack; users patch LFO-to-LFO constantly for evolving modulation; both VCV Fundamental LFO and Bogaudio LFO have FM
- Separate RESET jack — standard on VCV Fundamental LFO, Mutable Tides, nearly all hardware Eurorack LFOs; enables gate-driven phase restart independent of clock tracking
- Display text overlay readability — known v1.1 issue: HUD text becomes unreadable when the waveform trace passes through overlay areas
- Incoming clock BPM display — showing only ratio-adjusted BPM is misleading at non-x1 ratios; users need to verify clock detection is correct

**Should have (differentiators):**
- Phase offset knob with CV — enables quadrature patches (90-degree offset for stereo widening), polyrhythmic phase relationships, dynamic phase displacement via CV; few single-LFO modules offer a phase offset knob with CV
- Expanded analog imperfections (phase jitter, DC offset drift, pitch slew, component spread) — deepens the core module identity beyond pitch-only drift; no competing VCV LFO models this level of analog authenticity
- Waveform bleed during morph transitions — models analog crossfader crosstalk and capacitive coupling; no competing VCV LFO models this
- Swing/shuffle for clocked mode — genuine differentiator; no LFO module in VCV Rack has built-in swing; right-click menu approach means no panel control needed

**Defer to v1.2.1 or v1.3:**
- Swing/shuffle is the best deferral candidate if scope must be trimmed — highest complexity, most clock interactions, right-click implementation requires no panel changes

**Explicitly out of scope (anti-features):**
- Exponential FM mode toggle — perceptual difference from linear FM is negligible at LFO rates; adds complexity for zero musical benefit
- Through-zero FM — meaningful only at audio rates; produces confusing backward LFO modulation; defer to VCO module (v2.0)
- Individual drift sub-parameter knobs — violates the three-knob identity; all imperfections bundle under the existing Drift knob
- Separate phase-shifted output jack — contradicts single-output design philosophy
- Swing in free-running mode — without a clock, "swing" has no musical meaning

**Non-negotiable backward compatibility:** All new params default to neutral values. FM atten=0, phase offset=0, swing=50% produce output identical to v1.1. Drift=0 still means digital perfection for all new imperfections (all are gated by `drift >= 0.001f`).

### Architecture Approach

v1.2 extends the existing three-struct architecture (AnalogLFO module, WaveformDisplay widget, AnalogLFOWidget) without structural changes. All new computations insert at defined points in `process()`. The critical unifying principle across all new features is the accumulator-vs-readout distinction:

- **Accumulator:** phase, deltaPhase, clockBeatCount — the source of truth for timing. Never touched by jitter, phase offset, or component spread. Only modified by swing (deltaPhase) and reset events (snap to 0).
- **Readout:** the local `float p` copy computed just before `computeMorphedWave()`. Phase offset and jitter are applied here. The accumulator runs cleanly regardless.

No new display atomics are needed. The existing five atomics cover all new display data. `displayPhase` now stores the offset-inclusive phase value so the waveform dot accurately tracks position.

**Revised process() pipeline (v1.2):**
```
1.  processClockInput()              [unchanged]
2.  Process RESET input              [NEW: second SchmittTrigger]
3.  Compute targetFreq               [unchanged]
4.  Apply FM modulation              [NEW: targetFreq *= pow(2, fmVoltage * atten)]
5.  Apply drift-dependent pitch slew [MODIFIED: freqSlew lambda = f(drift)]
6.  Compute swing speed multiplier   [NEW: clocked mode only]
7.  deltaPhase = freq * sampleTime * swing
8.  Apply drift: OU layers modulate deltaPhase [unchanged]
9.  phase += deltaPhase; wrap [0, 1)  [unchanged]
10. Compute phaseOffset from knob+CV [NEW]
11. Apply phase jitter to readout    [NEW: p = phase + offset + jitter, never accumulator]
12. p = wrap(phase + phaseOffset + jitter, 1.0)
13. Read morph/character params + CV [unchanged]
14. Apply component spread to params [NEW: per-instance offsets * drift]
15. Compute bleedAmount from character [NEW]
16. computeMorphedWave(p, morph, character, bleed) [MODIFIED signature]
17. Apply DC offset drift             [NEW: reuses OU layer 0 state * drift * scaling]
18. Apply crossfade if recent reset  [unchanged]
19. Output voltage                   [unchanged: ±5V]
```

**Component boundary changes:**
- Phase accumulator: swing modulates deltaPhase; RESET adds second reset source
- Frequency computation: FM inserted before slew; slew lambda made drift-dependent
- Drift engine: DC offset drift reads existing OU layer 0 state — no new OU layers
- Waveform engine: `computeMorphedWave()` gains bleed parameter; called with readout phase (offset + jitter applied)
- Character engine: component spread adds per-instance parameter offsets scaled by drift
- Display renderer: HUD backgrounds via box gradient halo; clock BPM text; dot tracks offset-inclusive phase

### Critical Pitfalls

The research identified 15 pitfalls (6 critical, 6 moderate, 3 minor). The top 8 that most directly affect build decisions:

1. **FM driving frequency negative** — At LFO rates, FM easily pushes frequency through zero. `phase` goes unboundedly negative and waveform functions output garbage. Prevention: use exponential FM (`freq *= pow(2.f, fmVoltage * atten)`) which can never go negative, plus a safety clamp `max(freq, 0.001f)`. Add bidirectional phase wrap as defense-in-depth. Phase: FM implementation.

2. **FM fighting clock sync** — FM accumulates phase error between clock edges; at high depths, crossfade artifacts occur on every clock edge. Prevention: full-authority FM in clocked mode is a documented tradeoff — test and tune, or offer right-click "FM Mode: Frequency/Phase" to redirect FM to phase offset in clocked scenarios. Phase: FM, with clocked-mode test immediately.

3. **RESET + CLK simultaneous triggers** — Cable delay means the same source patched to both inputs arrives 1 sample apart. Without blanking: double phase reset and corrupted period measurement (1-sample period). VCV Rack Voltage Standards require 1ms post-RESET blanking for CLK triggers. Use `dsp::Timer`. RESET must not reset clock tracking state (`clockState`, `smoothedPeriod`). Phase: RESET implementation.

4. **Phase offset applied to accumulator, not readout** — Adding offset to the accumulator shifts when wrap occurs, breaking beat counting, swing half-cycle detection, display tracking, and causing clicks on offset changes. Prevention: keep accumulator clean; add offset at waveform lookup: `float p = fmod((float)phase + phaseOffset, 1.0f)`. Phase: phase offset implementation.

5. **Phase jitter accumulating against clock sync** — If jitter is added to the accumulator, floating-point bias compounds over 22,000+ samples per beat and causes visible phase drift despite clock sync. Prevention: apply jitter only to the readout copy `p`, never the accumulator. Scale jitter authority down in clocked mode (same 2% vs 7.5% pattern as drift authority). Phase: expanded imperfections.

6. **Waveform bleed causing amplitude spikes** — Adding non-adjacent waveform contributions without normalizing weights breaks the [−1, +1] output range. Prevention: normalize bleed coefficients so all weights sum to 1.0, or keep bleed small enough (3–8%) that overshoot is negligible; apply output clamp after bleed. Phase: waveform bleed.

7. **Pitch slew double-stacking with existing freqSlew** — A new pitch slew layer on top of the existing 50ms mode-transition filter makes rate changes feel sluggish. Prevention: apply pitch slew only to the OU drift output (not main frequency); use a much shorter time constant (2–5ms); gate on Drift knob. Phase: expanded imperfections.

8. **Display HUD text backgrounds — hard rectangle visual regression** — Known v1.1 issue: solid dark rectangle clips waveform glow, creating an "ugly black box." Prevention: `nvgBoxGradient()` soft-edged halo fading from opaque center (alpha 0.85) to transparent edge over ~4px. Draw order must be: waveform trace → background halos → text glow passes. Phase: display polish (Phase 1).

---

## Implications for Roadmap

Research across all four files converges on a 7-phase build order. The ordering is driven by data-flow dependencies (upstream features before downstream), interaction risk (tightly coupled features built together), and the v1.0 retrospective lesson (panel layout designed once for the final component count, never incrementally).

### Phase 1: Display Polish
**Rationale:** Zero DSP risk. Fixes the known v1.1 HUD readability regression. Establishes the display baseline before new features add new overlay elements. Quick win that ships an immediate UX improvement.
**Delivers:** Readable text overlays at all waveform positions via soft-edged halo backgrounds; incoming clock BPM displayed alongside effective BPM with clear labeling ("CLK 120" prefix or smaller/dimmer formatting to avoid ambiguity).
**Addresses:** Display text readability (table stakes), incoming clock BPM (table stakes).
**Avoids:** Pitfall 11 (hard rectangle visual regression — use box gradient halo), Pitfall 12 (ambiguous BPM labeling — only show raw clock BPM when ratio is not x1).
**Research flag:** Standard NanoVG patterns — no additional research needed.

### Phase 2: RESET Jack + Phase Offset
**Rationale:** These two features are tightly coupled: the reset target semantics depend on phase offset being defined first. Building them separately forces a two-pass rework. Both modify only the phase readout path; neither requires upstream DSP changes to be in place.
**Delivers:** Independent phase reset from rising-edge trigger (reuses existing crossfade); CV-modulatable phase shift (0–360 degrees) for quadrature patches and dynamic phase displacement. Reset always snaps to the offset position, not to 0.
**Addresses:** RESET jack (table stakes), phase offset knob/CV (differentiator).
**Avoids:** Pitfall 3 (1ms CLK/RESET blanking via `dsp::Timer`; RESET does not affect clock tracking state), Pitfall 5 (reset target = phaseOffset value because offset is applied at readout, not accumulator).
**Research flag:** Standard VCV Rack patterns — no additional research needed.

### Phase 3: FM Input
**Rationale:** FM is independent of Phase 2 features and modifies frequency computation — upstream of all phase/waveform logic. Implementing it third means the stable phase readout path from Phase 2 is already in place when the FM + phase offset PM interaction is tested.
**Delivers:** Exponential frequency modulation (`freq *= pow(2.f, fmVoltage * atten)`) via bipolar CV input with unipolar attenuator trimpot; works in both free and clocked modes.
**Addresses:** FM input jack (table stakes).
**Avoids:** Pitfall 1 (negative frequency via exponential FM + safety clamp), Pitfall 2 (FM vs clock sync — test and tune clocked-mode FM authority explicitly during this phase).
**Research flag:** Exponential vs linear FM at LFO rates is well-understood. One open design decision to resolve: in clocked mode, should FM have full frequency authority (expressive but may cause crossfade artifacts at high depths) or redirect to phase offset (cleaner sync but different sonic character)? This must be decided before implementation begins.

### Phase 4: Expanded Analog Imperfections
**Rationale:** Phase jitter, DC offset drift, pitch slew, and component spread all share the drift-scaling pattern (`progressiveCurve(drift)`) and insert into the same pipeline region. Implementing as a batch enables group testing for their interactions. Phase jitter must layer onto the readout path established in Phase 2; pitch slew must NOT stack with the existing `freqSlew` filter.
**Delivers:** Phase jitter (timing uncertainty at cycle level), DC offset drift (output center wander up to ±0.1V), pitch slew (frequency change inertia on OU drift output only, 2–5ms time constant), component spread (per-instance waveform personality generated in constructor, scaled by drift).
**Addresses:** Expanded analog imperfections (differentiator), module core identity deepening.
**Avoids:** Pitfall 4 (DC offset bounded to ±0.1V max; applied before 5V scaling), Pitfall 5b (jitter on readout copy only, never accumulator), Pitfall 7 (component spread effects visible on waveform display), Pitfall 8 (pitch slew on OU drift output only), Pitfall 9 (jitter scaled down in clocked mode), Pitfall 13 (independent RNG draws per imperfection type — do not share OU layer states between different imperfection types).
**Research flag:** Component spread perceptibility at LFO rates is explicitly flagged (Pitfall 7) as a design risk — the v1.0 retrospective noted analog character effects needed 3–5x larger values than research suggested to be perceptible at LFO rates. Plan an empirical tuning pass on jitter amplitude, DC offset scaling, and component spread magnitudes. Do not assume research values are final.

### Phase 5: Waveform Bleed
**Rationale:** Modifies `computeMorphedWave()` — a core function called from both `process()` and `updateDisplayBuffer()`. Changing this function signature is cleanest after Phases 2–4 have settled all the pipeline changes that feed into it.
**Delivers:** Adjacent and non-adjacent waveform bleed scaled by Character knob (`progressiveCurve(character) * 0.08f` maximum); visible in waveform display trace. At Character=0, morph is crisp crossfade (existing behavior). At Character=1, adjacent shapes bleed ~5–8%.
**Addresses:** Waveform bleed during morph transitions (differentiator).
**Avoids:** Pitfall 10 (normalize bleed coefficients so weights sum to 1.0; apply output clamp after bleed; keep bleed amounts small enough that unnormalized overshoot is negligible).
**Research flag:** Standard waveform math — no additional research needed. Bleed percentage (0.5–3% in real hardware) is LOW confidence; treat the 8% maximum as a design parameter to tune empirically.

### Phase 6: Swing/Shuffle
**Rationale:** Most complex feature. Interacts with clock sync, phase reset, phase offset, and division ratios. All prior DSP phases must be stable before swing layers on top. Swing's `inFirstHalf` detection uses raw `phase` (before offset) so offset and swing compose cleanly. Swing is clocked-mode only — right-click menu implementation means no panel control is needed.
**Delivers:** Swing/shuffle for clocked mode via right-click menu presets (50%, 54%, 58%, 62%, 67%, 71%, 75%); implemented as phase warp function applied after accumulation but before readout; display shows "SWG XX%" overlay when swing is non-50% and clocked.
**Addresses:** Swing/shuffle (differentiator).
**Avoids:** Pitfall 6 (phase warp function — not frequency modulation, not accumulator modification; piecewise linear warp is continuous and monotonic; identity at 50%; only active in ACQUIRING/LOCKED clock state).
**Research flag:** Two items need design decisions before implementation: (1) swing subdivision semantics at non-x1 ratios — does one swing cycle equal one LFO cycle or one clock beat? The musical intent differs; (2) smooth vs piecewise-linear warp at transition points — TR-909 uses hard piecewise-linear (authentic); sinusoidal ease sounds smoother. Choose intentionally.

### Phase 7: Panel Redesign
**Rationale:** Panel layout must be designed for the final component count — not incrementally modified. The v1.0 retrospective explicitly flagged incremental panel changes as a major source of wasted effort ("bottom row layout went through three iterations"). With all features finalized, the exact counts are known: 3 new jacks (FM, RESET, PHASE_OFFSET_CV), 3 new controls (PHASE_OFFSET knob, FM_ATTEN trimpot, SWING is right-click only — no panel control).
**Delivers:** Updated 12HP SVG panel accommodating all 6 new components; updated widget positions in `AnalogLFOWidget`.
**Addresses:** Panel density at 12HP (Pitfall 15).
**Avoids:** Pitfall 15 (design once for final state; if 12HP is too cramped, fallback priority is: remove FM attenuverter trimpot first, then defer Phase Offset CV, then expand to 14HP).
**Research flag:** Physical mm feasibility — sketch positions on a 60.96mm-wide grid before committing to SVG. Minimum 7mm center-to-center for adjacent jacks.

### Phase Ordering Rationale

- Display first: zero DSP risk; fixes a known regression; no dependencies on any new feature; ships visible improvement immediately
- RESET and Phase Offset together: reset target semantics depend on phase offset being co-designed; building separately forces rework
- FM before imperfections: FM modifies frequency computation (upstream); imperfections modify phase readout (downstream); upstream must be stable first
- Imperfections before bleed: phase jitter applies at the same pipeline region where bleed reads; imperfections must be settled before the function signature changes
- Bleed before swing: modifying `computeMorphedWave()` signature is cleanest when the surrounding pipeline is fully stable
- Swing last of DSP phases: depends on clock system, phase offset, and division counting all being stable; best deferral candidate if scope must be trimmed
- Panel always last: direct lesson from v1.0 retrospective

### Research Flags

Phases requiring explicit design decisions before implementation begins:
- **Phase 3 (FM):** Decide FM authority in clocked mode before writing code — full frequency authority vs. redirect to phase offset (see Gaps section).
- **Phase 4 (Imperfections):** Component spread magnitudes need empirical validation during implementation; budget a tuning pass.
- **Phase 6 (Swing):** Resolve subdivision semantics at non-x1 ratios and warp curve shape (linear vs. smooth) before implementation.

Phases with standard patterns (no pre-phase research needed):
- **Phase 1 (Display Polish):** NanoVG box gradient is documented SDK usage.
- **Phase 2 (RESET + Phase Offset):** SchmittTrigger and phase readout modification are established patterns; 1ms blanking rule is in VCV Rack Voltage Standards.
- **Phase 5 (Waveform Bleed):** Pure waveform math; all four shapes already computed.
- **Phase 7 (Panel):** SVG panel workflow established from v1.1 CLK jack addition.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Zero new dependencies; all additions use verified SDK built-ins and C++17 standard library; `src/AnalogLFO.cpp` directly inspected at 890 lines with precise line-number analysis |
| Features | HIGH | Table stakes verified against VCV Fundamental LFO, Bogaudio, and Mutable Tides official documentation; anti-features grounded in established project philosophy and explicit v1.0/v1.1 design decisions |
| Architecture | HIGH | All integration points identified with precise pipeline positions; interaction matrix across all new features vs. all existing systems is explicit; no circular dependencies |
| Pitfalls | HIGH | Critical pitfalls derived from direct source code analysis at specific line numbers; VCV Rack Voltage Standards verification; well-understood DSP engineering principles; v1.0/v1.1 retrospective lessons |

**Overall confidence:** HIGH

### Gaps to Address

- **FM authority in clocked mode:** Research documents both options (full-authority FM vs. phase-offset redirect in clocked mode) but does not pick one. This is a design decision, not a research gap. Decide before Phase 3 begins. The tradeoff: full authority is more expressive and musically useful (documented as desired behavior in FEATURES.md) but causes larger crossfade artifacts at high FM depths; phase-offset redirect is cleaner but changes the sonic character of clocked-mode FM. Recommendation: implement full-authority FM, test explicitly, and offer the redirect as a right-click option.

- **Component spread perceptibility at LFO rates:** Research suggests ±2–3% parameter offsets based on analog component tolerance specs. The v1.0 retrospective lesson ("character deformation amplitudes from research were too subtle — had to increase 3–5x") applies directly. Treat the research values as starting points, not final values. Plan an empirical tuning pass during Phase 4 before the feature is considered complete.

- **Swing subdivision semantics at non-x1 clock ratios:** At x1, one swing cycle = one LFO cycle (clear). At /4, it is ambiguous whether swing operates at the LFO cycle level (4 beats per swing cycle) or the clock beat level (swing occurs every 2 beats regardless of LFO rate). The musical intent differs and determines the implementation. Resolve with a design decision before Phase 6.

- **12HP panel feasibility with 6 new components:** Research identifies the layout as "tight but feasible" with specific mm proposals but does not verify them against actual coordinate constraints. Do a paper or SVG sketch before committing to implementation. The recommended fallback priority if 12HP is too cramped: (1) SWING is already right-click only, (2) remove FM attenuverter trimpot (fixed depth), (3) defer Phase Offset CV to v1.3, (4) expand to 14HP.

- **Component spread JSON versioning:** The decision to serialize the RNG seed rather than the spread values means the spread generation algorithm must be stable. If the algorithm changes between v1.2 and v1.3, old patches will silently produce different component spread. Consider adding a version field to the JSON alongside the seed.

---

## Sources

### Primary (HIGH confidence)
- `src/AnalogLFO.cpp` (local codebase, 890 lines, directly analyzed) — all integration points, enum layouts, existing filter parameters, specific line numbers for pitfall analysis
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) — 1V/Oct, trigger thresholds (0.1V/1.0V), 1ms RESET/CLK blanking rule
- [VCV Rack API: dsp namespace](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) — SchmittTrigger, ExponentialFilter, exp2_taylor5
- [VCV Library - VCV LFO](https://library.vcvrack.com/Fundamental/LFO) — FM input, RESET input feature verification
- [VCV Fundamental LFO source](https://github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp) — FM and RESET implementation reference
- [Mutable Instruments Tides documentation](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) — phase shift, FM CV, trigger reset
- [NanoVG API: nanovg.h](https://github.com/memononen/nanovg/blob/master/src/nanovg.h) — nvgTextBounds, nvgRoundedRect, nvgBoxGradient
- [Jatin Chowdhury: Bad Circuit Modelling — Component Tolerances](https://ccrma.stanford.edu/~jatin/Bad-Circuit-Modelling/Tolerances.html) — truncated Gaussian per-instance variation
- Project v1.0/v1.1 retrospectives — architectural lessons: panel layout rework, perceptibility amplitudes, OU drift authority

### Secondary (MEDIUM confidence)
- [KVR Audio: Implementing swing](https://www.kvraudio.com/forum/viewtopic.php?t=261858) — swing/shuffle DSP approaches, TR-909 shuffle timing values
- [Cycling '74: Shuffle for phasor LFO](https://cycling74.com/forums/shuffle-or-swing-for-a-phasor-driven-lfo) — phase warping approach for LFO swing
- [Frap Tools: Exponential FM Explained](https://frap.tools/frequency-modulation-part-1-exponential-fm/) — why exponential FM is correct for pitch modulation
- [Bogaudio GitHub](https://github.com/bogaudio/BogaudioModules) — LFO V/Oct, 8FO phase output patterns
- [Analog Devices: Temperature Drift](https://analogtoolshub.com/temperature-drift-analog-circuits/) — DC offset drift mechanisms, temperature coefficient specs
- [VCV Community: FM CV levels](https://community.vcvrack.com/t/fm-cv-levels-correspond-to-what-exactly/13561) — FM voltage scaling conventions
- [Mod Wiggler: LFO with sync and reset](https://www.modwiggler.com/forum/viewtopic.php?t=222452) — Eurorack RESET jack conventions

### Tertiary (LOW confidence — validate during implementation)
- Waveform bleed percentages (0.5–3% in real hardware) — general CD4066 analog switch knowledge, not measured from specific vintage synths
- Component spread magnitudes for LFO perceptibility (±2–3%) — standard EIA tolerance specs, not LFO-specific perceptual measurements; likely needs empirical increase
- Phase jitter magnitude (0.01–0.5% of cycle) — general oscillator knowledge, not measurements from specific vintage circuits

---
*Research completed: 2026-03-13*
*Ready for roadmap: yes*
