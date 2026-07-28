# Phase 30: VcoCore Skeleton & Module Registration - Context

**Gathered:** 2026-07-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Put a **real but deliberately aliased** oscillator body inside the Phase-29 POD seam, and **register the VCO as a second module** so it appears and makes sound in Rack. This proves the whole architecture end-to-end — POD boundary → harness → Rack shell → audible output — before any hard DSP lands.

Delivers:
1. `forge::VcoCore::step()` (`src/dsp/VcoCore.hpp`) with a naive audio-rate morphed oscillator replacing the Phase-29 silent stub — POD `VcoInputs` → `step()` → output + `Telemetry` boundary **unchanged in shape** (CORE-01).
2. Per-voice self-containment with **no static/global mutable voice state**, proven behaviorally (CORE-03) — v2.1 polyphony stays an additive shell change.
3. `src/AnalogVCO.cpp` — a minimal Rack shell delegating `process()` to `core.step()`, with a throwaway stub panel.
4. Registration as a second module: `addModel` in `plugin.cpp`, `extern Model* modelAnalogVCO` in `plugin.hpp`, a second `modules[]` entry in `plugin.json` (PANEL-03) — LFO entry byte-unchanged.
5. Deletion of the Phase-29 TOMBSTONE test and re-evidencing of the two invariants Phase 29 recorded as *green-but-weak*.

**Requirements:** CORE-01, CORE-03, PANEL-03.

**NOT in this phase (deliberate boundaries):**
- Coarse/fine tune, FM summing, the `< 1 cent` tracking gate (TEST-02) → **Phase 31** (PITCH-01..05, FM-01..03).
- `MorphBlep.hpp` / anti-aliasing → **Phase 32** (CORE-02, AA-01..05, TEST-03). The Phase 30 oscillator **aliases on purpose**.
- Hard sync → **Phase 33** (SYNC-01/02). `VcoInputs` still carries no sync fields.
- OU drift stepping, CV + attenuverters, the output stage (DC blocker + soft saturation) → **Phase 34** (CHAR-01, DRIFT-01..03, OUT-01..03).
- The real 18HP Forge Noir panel + CRT display → **Phase 35** (PANEL-01/02, DISP-01..03).
- VCO goldens, version bump, tag, #929 update → **Phase 36** (TEST-05, REL-01).

</domain>

<decisions>
## Implementation Decisions

### Module Identity & Manifest (PANEL-03 — permanent, one-way door)
- **D-01: Permanent slug is `ForgeAnalogVCO`.** Direct mirror of the shipped `ForgeAnalogLFO`. Full patch identifier: `ForgeAudio-AnalogSeries/ForgeAnalogVCO`. This is written into every user patch that ever contains the module and can never change.
- **D-02: Display name is `Analog VCO`**, matching "Analog LFO".
- **D-03: `plugin.json` tags are `["Voltage-controlled oscillator", "Waveshaper"]`** — structural mirror of the LFO's two tags with the oscillator class swapped. Waveshaper is earned by the morph/character engine. Explicitly **not** `Hardware clone` (VCV reserves it for actual clones; the character targets are references, and a mis-tag risks a reviewer objection on #929) and explicitly **not** `Polyphonic` (v2.0 ships monophonic; the tag would be a user-facing lie until v2.1 lands POLY-01).
- **D-04: `plugin.json` `version` stays `2.0.1` through Phase 30.** Phase 30 adds only the `modules[]` entry. Phase 36 owns REL-01 (bump + tag + #929 update). Rationale: the working tree never claims a release that was not cut, and no intermediate phase touches the field the Library keys on. A `-dev` suffix was rejected — VCV's manifest validator expects plain `MAJOR.MINOR.PATCH`.
- **D-05: Registration is strictly additive and operator-surfaced.** The `plugin.cpp` / `plugin.hpp` / `plugin.json` diff is presented to the operator before commit, per the milestone guardrail. The LFO's `addModel(modelAnalogLFO)` line, its `extern`, and its `modules[]` entry are byte-unchanged.

### Stub Shell Scope (Phase 30 vs Phase 35)
- **D-06: Throwaway `res/AnalogVCO.svg` at the final 18HP width.** A plain dark rectangle with a text label — deliberately ugly, zero Forge Noir design work. Establishing the real panel *filename* and *HP* now makes Phase 35 an art swap rather than a rewiring. Reusing `res/AnalogLFO.svg` was rejected (LFO labels on every VCO control is actively misleading during in-Rack checks, and it couples the VCO to a shipped asset the guardrail wants left alone).
- **D-07: Only the controls the Phase-30 DSP consumes get declared.** That is: **V/OCT input, MORPH knob, CHARACTER knob, OUT jack.** Every visible control does something, so an in-Rack check is honest — if a knob moves, you hear it. Later phases add their own controls alongside the behavior. Declaring the full PANEL-02 enum early was rejected: nothing has shipped, so no user patches exist, so param/input **ID churn is free right now** and ID-stability buys nothing.
- **D-08: Stock Rack widgets (`RoundBlackKnob`, `PJ301MPort`), not the Forge Noir components.** The Forge components are local structs inside `src/AnalogLFO.cpp`; reusing them would require extracting them out of a shipped LFO file. **Phase 30's diff therefore never touches `src/AnalogLFO.cpp` at all** — the cleanest possible story against the milestone guardrail.
- **D-09: No display widget.** DISP-01..03 are Phase 35.
- **D-10:** CHARACTER is in the Phase-30 control set as a **derived consequence of D-11**, not as a pull-forward of CHAR-01. Every component-spread coefficient in `Waveshape` is gated behind `character >= 0.001f`, so spread-driven divergence is unobservable at character = 0. Phase 34 still owns CHARACTER's CV input, attenuverter, and the drift engine.

### Naive DSP Scope & Seed Divergence
- **D-11: Seed divergence comes from component spread only — no OU drift stepping.** Mirror `LfoCore::setSpreadSeed`: copy the seed-derived spread coefficients (`triAsymmetrySpread`, `sawCurvatureSpread`, `squareDutySpread`, `pulseEdgeSpread`, `bleedSpread`) from `DriftEngine` into the `Waveshape` instance. Different spread seed → measurably different waveform, permanently, with **no per-sample RNG draws**. This models static per-instance analog variation and leaves the entire moving-drift engine to Phase 34, whose few-cents authority is an audition-gated operator decision (DRIFT-03) that must not be pre-empted by a naive first pass.
  - *This is what makes roadmap success criterion 4's "different seed diverges" half true.* Without it, a naive oscillator is a pure function of its inputs and is bit-identical across all seeds.
- **D-12: `VcoCore` gains a `Waveshape` member and calls `morphedWave(phase, morph, character, bleedLfo = 0.f)`** on the **frozen** `Waveshape.hpp` — a call, never an edit. `bleedLfo = 0` is correct here: it is the OU-layer-0 read, and no OU layer is being stepped (D-11).
- **D-13: Output is `×5` to ±5V with no conditioning.** No DC blocker, no soft saturation, no hard clamp — Phase 34 owns OUT-01..03. Character shaping adds harmonics on top of the base shapes, so raw `morphedWave` can exceed ±1 and the scaled output can overshoot ±5V at high character. That overshoot is **expected and audible on purpose** — it is the behavior Phase 34's output stage exists to fix, and hiding it behind a naive clamp would both conceal it and create work Phase 34 must undo. Phase 30 asserts a **loose** bound, never ±5V.
- **D-14: Pitch chain = reference + exp2 + double phase + Nyquist guard.** `freq = 261.6256f * exp2_taylor5(pitchCV)` — the same `C4 = 0V` reference Phase 31 will prove to `< 1 cent`, using `forge::exp2_taylor5` verbatim (never libm). Phase accumulated in **double precision**, mirroring `LfoCore`. A frequency clamp just below Nyquist is included as **safety, not scope**: without it a high V/OCT makes the naive accumulator produce meaningless garbage that would muddy every Phase-30 test. Coarse, fine, FM summing, and the tracking proof all stay in Phase 31.

### Test Evidence (replacing the Phase-29 tombstone)
- **D-15: The TOMBSTONE case is inverted, not merely deleted.** Same test slot, opposite assertion: the swept block is **not** all-zero and **not** constant. The Phase 29 → 30 transition then shows up as one readable diff line, and a future refactor cannot silently revert the core to a stub without failing loudly.
- **D-16: Pitch is checked by a loose zero-crossing period count on the OUTPUT**, at a few V/OCT values, within a few percent. Measuring the output rather than `tel.freqHz` is the load-bearing part: a telemetry assertion only re-reads the number `step()` just computed and would stay green even if the phase accumulator ignored the frequency entirely — precisely the vacuous-coverage trap Phase 29 called out. The test must be **explicitly labelled "not the tracking gate"** so Phase 31 cannot mistake it for TEST-02 coverage.
- **D-17: CORE-03 is enforced by a two-instance independence test.** Drive two differently-seeded `VcoCore` instances **interleaved, sample by sample**, and assert each produces exactly what it produces when run alone. Any shared mutable state — static, global, or an accidentally shared engine — makes the interleaved runs differ. This tests the actual property polyphony needs rather than a source-text proxy; a grep/nm guard was considered and judged to catch only the obvious declaration form.
- **D-18: Two additions beyond re-evidencing the weak pair.** (a) A **divergence** case — a different spread seed produces a measurably different block (the direct proof of criterion 4); (b) a **loose magnitude bound** well outside ±5V, catching a runaway accumulator or sign error rather than pinning the unconditioned output stage. Net: five real invariants (non-silence, pitch count, determinism, divergence, bound + finiteness) replace four weak ones.
- **D-19: The two Phase-29 "green-but-weak" rows are re-evidenced, not re-asserted.** Seam determinism and output finiteness become genuinely load-bearing once real DSP runs under `VcoBlockDriver::sweepScenario`. Phase 30's verification must state this explicitly — Phase 29's STATE.md entry records them as *green-but-weak, NOT coverage*, and that debt closes here.

### Claude's Discretion
- Exact stub-panel geometry, control placement, and param ranges (MORPH/CHARACTER are both `[0,1]`) — planner's call; the panel is throwaway.
- Tolerance constants: the zero-crossing period-count percentage (D-16), the loose magnitude bound (D-18b), and the "measurably different" threshold for the divergence case (D-18a). Pick values that are clearly non-vacuous but not brittle against an unconditioned, aliased output.
- Whether the Nyquist guard (D-14) is a hard clamp on `freqHz` or on the phase increment, and where the constant lives.
- Whether the new invariants extend `tests/test_vco_harness.cpp` or land in a new `tests/test_vco_core.cpp`. Either is fine — `make test` globs `tests/*.cpp`.
- Whether the existing guard scripts (`tests/check_includes.sh`, `check_canary.sh`, `check_frozen.sh`) need updating now that `src/AnalogVCO.cpp` exists. Note D-06's dependency-direction rule: `AnalogVCO.cpp` may include VCO headers freely, but no LFO TU may include a VCO-only file. `plugin.hpp` gaining `extern Model* modelAnalogVCO;` is a Rack symbol declaration, not a VCO-header include, and does not violate the audit.
- Whether the Phase-29 compile canary (`src/vco_compile_canary.cpp`) can retire now that `AnalogVCO.cpp` genuinely ODR-uses the VCO headers via the same `src/*.cpp` globs. **Recommendation: keep it.** D-08 of Phase 29 declared the canary permanent and growing, and it is the artifact that made the CI MinGW link gate *observably* bite (run 30339957128). Retiring it trades a proven tripwire for one unused symbol.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase 29 hand-off (read first — this phase inherits its seam and its debts)
- `.planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-CONTEXT.md` — D-01 (bare POD seam, `step()` silent by design), D-04..D-06 (the three hard-fail tripwires), D-07/D-08 (the permanent compile canary).
- `.planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-VERIFICATION.md` — what Phase 29 proved and what it explicitly did not.
- `.planning/STATE.md` §Accumulated Context — the Phase-29 entries, especially: the P-7 "green-but-weak" TEST-01 rows that Phase 30 must re-evidence; the TOMBSTONE mandate; the R-9 `VcoInputs`-not-`Inputs` ODR landmine; the standing "no tag on local evidence alone" rule.

### v2.0 VCO research (locks approach)
- `.planning/research/ARCHITECTURE.md` — VCO architecture: POD boundary, harness design, invariant mapping, guardrail encoding.
- `.planning/research/PITFALLS.md` — Rack-free include hygiene, the `(0,0)` Xoroshiro fixed point, C++11/ODR traps, `std::normal_distribution` portability.
- `.planning/research/SUMMARY.md` — research overview + phase-ordering rationale.
- `.planning/research/STACK.md` — toolchain/stack constraints.

### Requirements & roadmap
- `.planning/REQUIREMENTS.md` — CORE-01, CORE-03, PANEL-03 (this phase); CORE-02 (Phase 32); PITCH-*/FM-* (Phase 31); CHAR/DRIFT/OUT (Phase 34); PANEL-01/02 + DISP-* (Phase 35); POLY-01 (v2.1, enabled by CORE-03).
- `.planning/ROADMAP.md` §"Phase 30" — goal, 4 success criteria, and the registration guardrail note. Plus the v2.0 milestone guardrail block.
- `.planning/PROJECT.md` §Constraints — the LFO non-regression guardrail and the four frozen shared headers.

### Code to mirror and code to call
- `src/dsp/VcoCore.hpp` — **the file this phase modifies.** Its banner carries the binding C++11 rules (no `inline constexpr` variables, no `if constexpr`, no `std::clamp`, no in-class `static constexpr` indexed at runtime, no brace value-list init of `VcoInputs`). Read the banner before editing.
- `src/dsp/LfoCore.hpp` — the boundary and orchestration precedent: POD → `step()` → float + `Telemetry`, `setSpreadSeed` copying spread coefficients into `Waveshape` (the D-11 pattern), double-precision phase, zero `rack/` includes.
- `src/dsp/Waveshape.hpp` — **FROZEN. Call, never edit.** `morphedWave(phase, morph, character, bleedLfo)`; the five spread coefficient members; note every spread is gated behind `character >= 0.001f`.
- `src/dsp/RackCompat.hpp` — **FROZEN.** `forge::exp2_taylor5` (bit-identical to the Rack SDK polynomial — use it, never libm), `forge::clamp` / `clampi`, `forge::Xoroshiro128Plus`.
- `src/dsp/DriftEngine.hpp` — **FROZEN in Phase 30** (Phase 34 is the only sanctioned edit). `seed()` / `setSpreadSeed()` and the spread coefficient members.
- `src/dsp/MathConst.hpp` — **FROZEN.** `forge::kPi`.
- `tests/VcoBlockDriver.hpp` — the harness. Non-zero default seeds, `sampleTime`/`sampleRate` overwritten every step, `sweepScenario()`. **Never template or subclass it with `tests/BlockDriver.hpp`** (R-2/P-4 — that file feeds the shipped LFO's bit-exact golden leg).
- `tests/test_vco_harness.cpp` — the TOMBSTONE (case 7) to invert per D-15, and the weak invariants (cases 5 and 6) to re-evidence per D-19.
- `src/AnalogLFO.cpp:1212` — `createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO")`, the registration pattern to mirror. **Do not edit this file.**
- `src/plugin.cpp`, `src/plugin.hpp`, `plugin.json` — the three additive registration edits (D-05).
- `res/components/` — the shared Forge Noir component SVGs. Not used by the Phase-30 stub (D-08), but relevant to the deferred knob work.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`src/dsp/LfoCore.hpp`** — the direct template for everything structural in this phase: the `setSpreadSeed` → `Waveshape` coefficient copy (D-11 is literally this pattern), double-precision phase accumulation, and the shell-delegates-to-`core.step()` split.
- **`src/dsp/Waveshape.hpp::morphedWave`** — the entire waveform body of Phase 30 is one call. The 5-shape crossfade, the `morph × 4` segment rescale, the duty-interpolated square→pulse region, and the bleed ring are all already written and frozen.
- **`tests/VcoBlockDriver.hpp::sweepScenario`** — already sweeps `pitchCV` (−2..+2 V), `morph` (0..1), and `character` (1..0) across a block. It was built in Phase 29 explicitly so the weak invariants would become load-bearing the moment Phase 30 landed DSP. It needs no changes.
- **Makefile / CI globs** — `make test` globs `tests/*.cpp` + `src/dsp/*.hpp`; `make strict` and the CI MinGW link leg glob `src/*.cpp`. **`src/AnalogVCO.cpp` and any new test file are picked up automatically — no build or CI wiring is needed.**

### Established Patterns
- Rack-free header-only DSP under `src/dsp/*.hpp` with **zero `rack/` includes**; the Rack shell owns params/inputs/outputs, display atomics, and JSON serialization, and delegates per-sample work to the core.
- Two-standard compilation: every VCO header must compile clean under **both** `-std=c++11 -pedantic-errors` (the shipped plugin toolchain) and `-std=c++17` (the test target). The C++11 restrictions in the `VcoCore.hpp` banner are binding.
- Hard-fail mechanical gates over conventions (the Phase-29 posture) — which is why D-17 chose a behavioral independence test over a documented rule.
- `-ffp-contract=off`, no `-ffast-math`, in the test build.

### Integration Points
- **`src/dsp/VcoCore.hpp`** — modified in place: `step()` body, a `Waveshape wave` member, and a `setSpreadSeed` that also populates the wave coefficients. The **boundary shape does not change**, so `VcoBlockDriver` and the harness keep working untouched.
- **`src/AnalogVCO.cpp`** — new file. Joins the `src/*.cpp` globs and is therefore covered by `make strict` and the CI MinGW link leg identically to how `vco_compile_canary.cpp` is today.
- **`src/plugin.cpp` / `src/plugin.hpp` / `plugin.json`** — three additive edits, operator-surfaced (D-05). The only files in this phase shared with the shipped LFO.
- **`res/AnalogVCO.svg`** — new throwaway asset, replaced wholesale in Phase 35.
- **The standing tripwires are untouched:** no frozen header is edited, so the `FROZEN.sha256` manifest needs no bump; the golden checksum lock is unaffected (no LFO behavior changes); the include-direction audit is satisfied because no LFO TU includes any VCO file.

</code_context>

<specifics>
## Specific Ideas

- **"Aliased on purpose" is a feature of this phase, not a defect to apologize for.** The Phase-30 oscillator is expected to sound crude at high notes. Phase 32 is the linchpin that fixes it. No test in Phase 30 should assert anything about spectral cleanliness.
- **Phase 30's diff must not touch `src/AnalogLFO.cpp`.** This was chosen deliberately (D-08) over the more convenient path of extracting the Forge Noir components — the cleanest possible position against the milestone guardrail is that the shipped module's source is not in the diff at all.
- **Vacuous coverage is the failure mode this project has already been bitten by.** Phase 29 recorded that its entire local gate returned exit 0 on code that could not link. D-16's insistence on measuring the output rather than telemetry, and D-15's inversion rather than deletion of the tombstone, both come from that lesson: a test that cannot fail is not evidence.
- **The slug is a one-way door and was treated as one.** `ForgeAnalogVCO` goes into every user patch containing the module, forever.

</specifics>

<deferred>
## Deferred Ideas

- **Knob redesign + LFO backport → its own dedicated phase, before Phase 35.** The operator is happy with the current jacks but considers the knobs the weakest part of the Forge Noir design, and wants new knob styles designed for the VCO and applied back to the shipped LFO. Mechanically this is cheap: knob art is **shared assets** — `res/components/ForgeKnob{Hero,Secondary}{,_bg}.svg` and `ForgeTrimpot*.svg`, loaded by widget structs — so a single asset edit updates both modules and the "backport" is automatic rather than a port. It does change the shipped module's appearance, so it needs an operator surface plus a visual confirmation that nothing else shifted. Placed before Phase 35 so the VCO panel is built around knobs the operator already likes, and given its own phase rather than folded into Phase 35 (which already carries PANEL-01/02 + DISP-01/02/03) so the visual iteration gets its own budget. **Action: insert via `/gsd-phase` before planning Phase 35.**
- Everything else VCO-behavioral was routed to its owning phase rather than pulled forward: coarse/fine/FM and the `< 1 cent` gate → Phase 31; anti-aliasing → Phase 32; hard sync → Phase 33; OU drift stepping, CV/attenuverters and the output stage → Phase 34; the real panel and display → Phase 35; goldens, version bump and the #929 update → Phase 36.

</deferred>

---

*Phase: 30-vcocore-skeleton-module-registration*
*Context gathered: 2026-07-28*
