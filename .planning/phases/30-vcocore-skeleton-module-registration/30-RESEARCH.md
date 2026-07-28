# Phase 30: VcoCore Skeleton & Module Registration - Research

**Researched:** 2026-07-28
**Domain:** VCV Rack 2 audio-rate oscillator core + second-module registration inside a live, library-published plugin
**Confidence:** HIGH

> **Method note.** Almost every number in this document was **measured in this session**, not recalled.
> A working Phase-30 prototype (`VcoCore::step()` DSP + `src/AnalogVCO.cpp` + registration + stub SVG)
> was built in an isolated scratch copy of the repository, compiled with `make strict`, linked with
> `make` against the real `../Rack-SDK`, and run through `make test` and `make guards`. Four numerical
> probes swept the `Waveshape` parameter space. **The real working tree was never modified**
> (`git status --porcelain` empty before and after). Findings tagged `[MEASURED]` are reproducible
> from the commands recorded in § Reproduction Commands.

---

<user_constraints>
## User Constraints (from 30-CONTEXT.md)

### Locked Decisions

**Module Identity & Manifest (PANEL-03 — permanent, one-way door)**
- **D-01: Permanent slug is `ForgeAnalogVCO`.** Direct mirror of the shipped `ForgeAnalogLFO`. Full patch identifier: `ForgeAudio-AnalogSeries/ForgeAnalogVCO`. This is written into every user patch that ever contains the module and can never change.
- **D-02: Display name is `Analog VCO`**, matching "Analog LFO".
- **D-03: `plugin.json` tags are `["Voltage-controlled oscillator", "Waveshaper"]`** — structural mirror of the LFO's two tags with the oscillator class swapped. Waveshaper is earned by the morph/character engine. Explicitly **not** `Hardware clone` (VCV reserves it for actual clones; the character targets are references, and a mis-tag risks a reviewer objection on #929) and explicitly **not** `Polyphonic` (v2.0 ships monophonic; the tag would be a user-facing lie until v2.1 lands POLY-01).
- **D-04: `plugin.json` `version` stays `2.0.1` through Phase 30.** Phase 30 adds only the `modules[]` entry. Phase 36 owns REL-01 (bump + tag + #929 update). Rationale: the working tree never claims a release that was not cut, and no intermediate phase touches the field the Library keys on. A `-dev` suffix was rejected — VCV's manifest validator expects plain `MAJOR.MINOR.PATCH`.
- **D-05: Registration is strictly additive and operator-surfaced.** The `plugin.cpp` / `plugin.hpp` / `plugin.json` diff is presented to the operator before commit, per the milestone guardrail. The LFO's `addModel(modelAnalogLFO)` line, its `extern`, and its `modules[]` entry are byte-unchanged.

**Stub Shell Scope (Phase 30 vs Phase 35)**
- **D-06: Throwaway `res/AnalogVCO.svg` at the final 18HP width.** A plain dark rectangle with a text label — deliberately ugly, zero Forge Noir design work. Establishing the real panel *filename* and *HP* now makes Phase 35 an art swap rather than a rewiring. Reusing `res/AnalogLFO.svg` was rejected (LFO labels on every VCO control is actively misleading during in-Rack checks, and it couples the VCO to a shipped asset the guardrail wants left alone).
- **D-07: Only the controls the Phase-30 DSP consumes get declared.** That is: **V/OCT input, MORPH knob, CHARACTER knob, OUT jack.** Every visible control does something, so an in-Rack check is honest — if a knob moves, you hear it. Later phases add their own controls alongside the behavior. Declaring the full PANEL-02 enum early was rejected: nothing has shipped, so no user patches exist, so param/input **ID churn is free right now** and ID-stability buys nothing.
- **D-08: Stock Rack widgets (`RoundBlackKnob`, `PJ301MPort`), not the Forge Noir components.** The Forge components are local structs inside `src/AnalogLFO.cpp`; reusing them would require extracting them out of a shipped LFO file. **Phase 30's diff therefore never touches `src/AnalogLFO.cpp` at all** — the cleanest possible story against the milestone guardrail.
- **D-09: No display widget.** DISP-01..03 are Phase 35.
- **D-10:** CHARACTER is in the Phase-30 control set as a **derived consequence of D-11**, not as a pull-forward of CHAR-01. Every component-spread coefficient in `Waveshape` is gated behind `character >= 0.001f`, so spread-driven divergence is unobservable at character = 0. Phase 34 still owns CHARACTER's CV input, attenuverter, and the drift engine.

**Naive DSP Scope & Seed Divergence**
- **D-11: Seed divergence comes from component spread only — no OU drift stepping.** Mirror `LfoCore::setSpreadSeed`: copy the seed-derived spread coefficients (`triAsymmetrySpread`, `sawCurvatureSpread`, `squareDutySpread`, `pulseEdgeSpread`, `bleedSpread`) from `DriftEngine` into the `Waveshape` instance. Different spread seed → measurably different waveform, permanently, with **no per-sample RNG draws**. This models static per-instance analog variation and leaves the entire moving-drift engine to Phase 34, whose few-cents authority is an audition-gated operator decision (DRIFT-03) that must not be pre-empted by a naive first pass.
  - *This is what makes roadmap success criterion 4's "different seed diverges" half true.* Without it, a naive oscillator is a pure function of its inputs and is bit-identical across all seeds.
- **D-12: `VcoCore` gains a `Waveshape` member and calls `morphedWave(phase, morph, character, bleedLfo = 0.f)`** on the **frozen** `Waveshape.hpp` — a call, never an edit. `bleedLfo = 0` is correct here: it is the OU-layer-0 read, and no OU layer is being stepped (D-11).
- **D-13: Output is `×5` to ±5V with no conditioning.** No DC blocker, no soft saturation, no hard clamp — Phase 34 owns OUT-01..03. Character shaping adds harmonics on top of the base shapes, so raw `morphedWave` can exceed ±1 and the scaled output can overshoot ±5V at high character. That overshoot is **expected and audible on purpose** — it is the behavior Phase 34's output stage exists to fix, and hiding it behind a naive clamp would both conceal it and create work Phase 34 must undo. Phase 30 asserts a **loose** bound, never ±5V.
- **D-14: Pitch chain = reference + exp2 + double phase + Nyquist guard.** `freq = 261.6256f * exp2_taylor5(pitchCV)` — the same `C4 = 0V` reference Phase 31 will prove to `< 1 cent`, using `forge::exp2_taylor5` verbatim (never libm). Phase accumulated in **double precision**, mirroring `LfoCore`. A frequency clamp just below Nyquist is included as **safety, not scope**: without it a high V/OCT makes the naive accumulator produce meaningless garbage that would muddy every Phase-30 test. Coarse, fine, FM summing, and the tracking proof all stay in Phase 31.

**Test Evidence (replacing the Phase-29 tombstone)**
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

### Deferred Ideas (OUT OF SCOPE)
- **Knob redesign + LFO backport → its own dedicated phase, before Phase 35.** The operator is happy with the current jacks but considers the knobs the weakest part of the Forge Noir design, and wants new knob styles designed for the VCO and applied back to the shipped LFO. Mechanically this is cheap: knob art is **shared assets** — `res/components/ForgeKnob{Hero,Secondary}{,_bg}.svg` and `ForgeTrimpot*.svg`, loaded by widget structs — so a single asset edit updates both modules and the "backport" is automatic rather than a port. It does change the shipped module's appearance, so it needs an operator surface plus a visual confirmation that nothing else shifted. Placed before Phase 35 so the VCO panel is built around knobs the operator already likes, and given its own phase rather than folded into Phase 35 (which already carries PANEL-01/02 + DISP-01/02/03) so the visual iteration gets its own budget. **Action: insert via `/gsd-phase` before planning Phase 35.**
- Everything else VCO-behavioral was routed to its owning phase rather than pulled forward: coarse/fine/FM and the `< 1 cent` gate → Phase 31; anti-aliasing → Phase 32; hard sync → Phase 33; OU drift stepping, CV/attenuverters and the output stage → Phase 34; the real panel and display → Phase 35; goldens, version bump and the #929 update → Phase 36.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| **CORE-01** | A new Rack-free `forge::VcoCore` (`src/dsp/VcoCore.hpp`) mirrors the `LfoCore` POD-`Inputs` → `step()` → output+telemetry boundary | § Pattern 1 (the exact `step()` body, verified to compile at C++11 and to build+link against Rack-SDK); § Code Example 1. The boundary shape is already correct in the Phase-29 header — Phase 30 adds a body, two members and a `setSpreadSeed` extension, and changes **nothing** in `VcoInputs` or `Telemetry`. |
| **CORE-03** | `VcoCore` is a self-contained per-voice unit with no static/global mutable voice state — polyphony-ready | § Pattern 4 + § Finding 4 (the five measured non-vacuity requirements for the D-17 interleave test, plus a permanent in-test positive control that is *measured* to go red on a deliberately-shared accumulator). Prototype result: interleaved vs solo mismatches **A = 0/1024, B = 0/1024**, with soloA == soloB on **0/1024** samples. |
| **PANEL-03** | The VCO is registered as a second module (`addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry) without altering the LFO's registration | § Pattern 5 (the exact four-file additive diff, **verified by building `plugin.dylib` and confirming `nm` exports both `_modelAnalogLFO` and `_modelAnalogVCO`**); § Pattern 6 (stub panel SVG requirements, verified through the SDK's own vendored nanosvg). |
</phase_requirements>

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| V/oct → frequency (`exp2_taylor5`, C4 reference) | Pure DSP core (`src/dsp/VcoCore.hpp`) | — | Rack-free, headlessly testable, golden-capturable. The shell must never compute pitch. |
| Nyquist frequency guard | Pure DSP core | — | It needs `sampleRate`, which the POD already injects. Phase 31's PITCH-04 replaces the constant on the **same surface**. |
| Phase accumulation + wrap | Pure DSP core (`double phase` member) | — | Per-voice state. Making it a member (never a static) is literally what CORE-03 asserts. |
| Waveform shaping / morph / character | Frozen leaf (`src/dsp/Waveshape.hpp`) — **called**, never edited | Pure DSP core holds the `Waveshape` instance | D-12. The whole waveform body is one `morphedWave()` call. Editing the file would put the shipped LFO's goldens at risk. |
| Per-instance component spread | Frozen leaf (`src/dsp/DriftEngine.hpp`) → copied into `Waveshape` by `VcoCore::setSpreadSeed` | — | D-11, mirroring `LfoCore::setSpreadSeed` exactly. |
| ×5 output scaling | Pure DSP core | — | Matches `LfoCore` step 14. Conditioning (DC block, saturation) is Phase 34, **not here** (D-13). |
| Rack params/inputs/outputs marshalling | Rack shell (`src/AnalogVCO.cpp`) | — | The core never sees a Rack index. |
| Panel, widgets, HP | Rack shell + `res/AnalogVCO.svg` | — | `ModuleWidget::setPanel()` derives the widget size from the SVG. |
| Model registration | Rack shell + `src/plugin.cpp` / `src/plugin.hpp` / `plugin.json` | — | The only three files in this phase shared with the shipped LFO — D-05 operator surface. |
| Seeding entropy | Rack shell (forwards explicit seeds in) | — | ARCHITECTURE Anti-Pattern 3: no `random_device` inside `src/dsp/`. |
| Test drive + timing injection | Test layer (`tests/VcoBlockDriver.hpp`) — **unchanged** | — | The Phase-29 harness needs zero edits (verified). |

---

## Summary

Phase 30 is mechanically small and evidentially demanding. The DSP is roughly **eighteen lines** inside
an existing seam: one `exp2_taylor5`, one Nyquist clamp, one double-precision accumulate-and-wrap, one
call into the frozen `Waveshape::morphedWave`, one `×5`, three telemetry writes. The registration is a
four-file additive diff. Everything hard about this phase lives in the *test design* — choosing
tolerances and scenarios that are non-vacuous against a deliberately-aliased, deliberately-unconditioned
signal — and in one **guard-script collision that will hard-fail CI on the first commit** if it is not
handled.

That collision is the single most important finding here. `tests/check_includes.sh` section `[2/7]`
("VCO headers are Rack-free") matches any include whose path contains `[Rr]ack`. D-14 requires
`forge::exp2_taylor5`, which lives in `src/dsp/RackCompat.hpp`. Adding
`#include "dsp/RackCompat.hpp"` to `VcoCore.hpp` therefore trips a **false positive** — reproduced in
this session, `make guards` exits 1 — even though `tests/check_canary.sh` `[5b/5]` *explicitly lists
`RackCompat.hpp` as an allowed frozen shared header for the VCO seam*. Two guards in this repository
currently disagree about the same include. § Guard Script Impact carries the exact patch and the
lower-risk alternative.

The numbers the planner needs are all measured rather than estimated. The output has a **hard analytic
ceiling of ±5.55 V** (`5 × 1.11`, derived from the Chebyshev-shaped sine at full character and confirmed
by an exhaustive sweep) — so the D-18b "loose bound" should be **6.0 V**. The waveform has **exactly two
sign changes per cycle at all 40,501 tested (morph, character) points**, so zero-crossing period counting
is structurally sound — but it collapses (−47 % error) once the narrow-pulse region has under one sample
per pulse, which pins a concrete safe test envelope. Seed divergence is **exactly zero at character = 0**
and 0.14–0.48 V at character = 1, which makes `character > 0` a hard requirement for both the divergence
test *and* the independence test, not a stylistic choice.

**Primary recommendation:** land the DSP exactly as § Code Example 1 (it is verified to pass `make strict`,
build and link against Rack-SDK, and leave 66/67 tests green with the TOMBSTONE as the sole intended
failure); patch `check_includes.sh [2/7]` in the same commit and surface it to the operator alongside the
D-05 registration diff; put the five new invariants in a **new `tests/test_vco_core.cpp`** with the
measured constants from § Measured Constants; and **keep the compile canary** — retiring it would delete
three guard sections that key off its existence and would lose runtime coverage of five `VcoInputs` fields
that `AnalogVCO.cpp` never exercises.

---

## Standard Stack

No new dependency is added or needed. Every component already exists in the repository or the SDK.

### Core

| Component | Version / Location | Purpose | Why Standard |
|-----------|-------------------|---------|--------------|
| `forge::exp2_taylor5` | `src/dsp/RackCompat.hpp:112` (FROZEN) | V/oct → frequency | Bit-identical to the Rack SDK `approx.hpp` polynomial. D-14 mandates it; libm `exp2` would break the FM path's bit-identity discipline. `[VERIFIED: repo source]` |
| `forge::clamp` | `src/dsp/RackCompat.hpp:97` (FROZEN) | `morph`/`character` clamping | `std::clamp` is C++17 and hard-fails the toolchain gate — `check_canary.sh [4/5]` proves it does. `[VERIFIED: repo source + guard run]` |
| `forge::Waveshape::morphedWave` | `src/dsp/Waveshape.hpp:158` (FROZEN — call, never edit) | The entire waveform body | D-12. 5-shape crossfade, `morph × 4` segment rescale, duty-interpolated square→pulse, bleed ring — all already written and golden-pinned. `[VERIFIED: repo source]` |
| `forge::DriftEngine::setSpreadSeed` | `src/dsp/DriftEngine.hpp:96` (FROZEN in Phase 30) | Per-instance component spread | D-11. The five coefficients are copied into `Waveshape` exactly as `LfoCore::setSpreadSeed` does. `[VERIFIED: repo source]` |
| `rack::createModel<TModule, TWidget>(slug)` | `Rack-SDK/include/helpers.hpp:24` | Model factory | The only registration mechanism. `[VERIFIED: SDK source]` |
| `RoundBlackKnob`, `PJ301MPort` | `Rack-SDK` component library | Stub panel widgets (D-08) | Verified to compile and link in the prototype build. `[VERIFIED: prototype build]` |
| doctest | 2.4.11, vendored at `tests/doctest.h` | Test framework | Already the project's framework; `make test` globs `tests/*.cpp`. `[VERIFIED: repo source]` |

### Supporting

| Component | Location | Purpose | When to Use |
|-----------|----------|---------|-------------|
| `forge::VcoBlockDriver` | `tests/VcoBlockDriver.hpp` | Headless block driver + seeding + timing injection | Every new test. **Needs no changes** — verified: the Phase-29 harness drives the Phase-30 DSP unmodified. |
| `VcoBlockDriver::sweepScenario` | same | Anti-vacuity varying input | Determinism, finiteness, divergence, non-silence. **Not** sufficient for the magnitude bound — see § Finding 3. |
| `forge::kPi` | `src/dsp/MathConst.hpp` (FROZEN) | π | Already reached transitively via `Waveshape.hpp`; Phase 30 needs no direct use. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Clamping `freqHz` | Clamping `deltaPhase` | Both preserve the wrap invariant. Clamping `freqHz` keeps `tel.freqHz` honest about what was actually produced and matches PITCH-04's own wording ("Frequency is clamped just below Nyquist"), so Phase 31 replaces a constant rather than restructuring. **Recommend `freqHz`.** |
| Explicit `#include "dsp/RackCompat.hpp"` | Rely on the transitive include via `dsp/DriftEngine.hpp` | The transitive path compiles today and avoids the guard patch entirely, but it silently depends on `DriftEngine.hpp`'s include list and contradicts the repo's own include-what-you-use practice (`LfoCore.hpp` includes `RackCompat.hpp` explicitly even though `DriftEngine.hpp` would supply it). **Recommend the explicit include + guard patch**, operator-surfaced. |
| New `tests/test_vco_core.cpp` | Extend `tests/test_vco_harness.cpp` | The harness file's banner explicitly says pitch accuracy and output bounds are *deliberately not here*. Splitting keeps `-tc="vco harness*"` and `-tc="vco core*"` as separate filtered suites for VALIDATION.md. **Recommend a new file**, with only the D-15 tombstone inversion and the D-19 banner rewrites landing in the harness file. |
| Retiring `src/vco_compile_canary.cpp` | Keep it | See § Finding 7. **Keep** — retiring it deletes `check_canary.sh` `[2/5]`, `[2b/5]`, `[5/5]` and `[5b/5]`, and loses runtime-live coverage of 5 of 8 `VcoInputs` fields. |

**Installation:** none. No package manager is involved in this phase.

---

## Package Legitimacy Audit

**Not applicable — this phase installs zero external packages.** No `npm`, `pip`, `cargo`, or vendored
third-party code is added. Every dependency is either already in the repository (`src/dsp/*.hpp`,
`tests/doctest.h`) or is the already-present VCV Rack SDK at `../Rack-SDK`. There is therefore no
slopsquatting surface and no `checkpoint:human-verify` install gate to add.

---

## Architecture Patterns

### System Architecture Diagram

```
   ┌──────────── Rack audio thread ────────────┐        ┌──── make test (Rack-free, C++17) ────┐
   │                                            │        │                                       │
 [V/OCT jack] [MORPH knob] [CHARACTER knob]     │        │   tests/test_vco_core.cpp   (NEW)     │
        │           │             │             │        │   tests/test_vco_harness.cpp (edited) │
        └───────────┴──────┬──────┘             │        │                 │                     │
                           ▼                    │        │                 ▼                     │
              src/AnalogVCO.cpp  (NEW)          │        │   tests/VcoBlockDriver.hpp            │
              AnalogVCO::process()              │        │   (UNCHANGED — verified)              │
              marshals Rack I/O ──► POD         │        │   · seeds core                        │
                           │                    │        │   · overwrites sampleTime/Rate        │
                           ▼                    │        │   · sweepScenario()                   │
                 ┌───────────────────┐          │        └────────────┬──────────────────────────┘
                 │ forge::VcoInputs  │◄─────────┼─────────────────────┘
                 │ (POD, UNCHANGED)  │          │
                 └─────────┬─────────┘          │
                           ▼                    │
   ┌─────────────── src/dsp/VcoCore.hpp — step() ────────────────┐
   │  1. record telemetry (sampleTime / sampleRate / stepCount)   │
   │  2. freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV)             │
   │  3. NaN/neg guard, then clamp freq ≤ 0.49 * sampleRate  ◄── PITCH-04 replaces this in Ph.31
   │  4. tel.freqHz = freq                                        │
   │  5. deltaPhase = (double)freq * (double)sampleTime           │
   │  6. phase += deltaPhase ; if (phase >= 1.0) phase -= 1.0     │
   │  7. morph/character = forge::clamp(..., 0, 1)                │
   │  8. sample = wave.morphedWave(phase, morph, character, 0.f)  │──► src/dsp/Waveshape.hpp
   │  9. tel.displayPhase = phase                                 │    (FROZEN — call only)
   │ 10. return 5.f * sample     ◄── NO conditioning (D-13)       │
   └──────────────────────────────┬───────────────────────────────┘
                                  │  setSpreadSeed(s0,s1)
                                  ▼
                    src/dsp/DriftEngine.hpp (FROZEN)
                    5 spread coefficients ──► copied into the Waveshape member (D-11)

   Registration (additive, D-05):
     src/plugin.hpp  ── extern Model* modelAnalogVCO;
     src/plugin.cpp  ── p->addModel(modelAnalogVCO);
     src/AnalogVCO.cpp ── Model* modelAnalogVCO = createModel<…>("ForgeAnalogVCO");
     plugin.json     ── second modules[] entry
     res/AnalogVCO.svg ── 91.44mm × 128.5mm stub (18.00 HP)
```

Data flows one way: Rack I/O → POD → core → float volts + telemetry → Rack I/O. The core reads no
global, no Rack index, no OS entropy. The **only** cycle-shaped edge is `setSpreadSeed`, which runs at
construction/patch-load, never per sample.

### Recommended Project Structure

```
src/
├── AnalogLFO.cpp            # SHIPPED — NOT IN THIS PHASE'S DIFF AT ALL (D-08)
├── AnalogVCO.cpp            # NEW — auto-compiled by SOURCES += $(wildcard src/*.cpp)
├── vco_compile_canary.cpp   # KEEP (see Finding 7)
├── plugin.cpp               # +1 line
├── plugin.hpp               # +1 line
└── dsp/
    ├── VcoCore.hpp          # MODIFIED IN PLACE — body + 2 members + setSpreadSeed extension
    ├── Waveshape.hpp        # FROZEN — called
    ├── DriftEngine.hpp      # FROZEN in Phase 30
    ├── RackCompat.hpp       # FROZEN — called
    └── MathConst.hpp        # FROZEN
res/
└── AnalogVCO.svg            # NEW — throwaway 18HP stub, replaced wholesale in Phase 35
tests/
├── VcoBlockDriver.hpp       # UNCHANGED (verified)
├── test_vco_harness.cpp     # MODIFIED — D-15 tombstone inversion + D-19 banner rewrites
├── test_vco_core.cpp        # NEW — the five behavioral invariants
└── check_includes.sh        # MODIFIED — [2/7] false-positive fix (operator-surfaced)
plugin.json                  # +1 modules[] entry
```

### Pattern 1: The `step()` body — verified shape

The exact prototype used for every measurement in this document is in § Code Example 1. Two structural
constraints on that source are **mechanically enforced** by `tests/check_canary.sh [2b/5]`, which
line-matches the header in order to build a perturbed copy:

```bash
# tests/check_canary.sh:218 and :231 — these two case patterns must keep matching
"struct VcoCore"*"{"*)
*"float step(const VcoInputs& in)"*"{"*)
```

> **HARD CONSTRAINT.** `struct VcoCore {` must stay on **one line**, and
> `float step(const VcoInputs& in) {` must stay on **one line with its opening brace**.
> Reformatting either (Allman braces, a wrapped parameter list, an added `noexcept`) makes
> `check_canary.sh [2b/5]` fail with *"could not perturb src/dsp/VcoCore.hpp"* — a hard `make guards`
> failure. Verified: the prototype preserved both shapes and `[2b/5]` reported
> *"all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3"*. `[MEASURED]`

### Pattern 2: The Nyquist guard clamps `freqHz`, not `deltaPhase`

```cpp
float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);
const float maxFreq = kVcoNyquistGuardFrac * in.sampleRate;   // 0.49f
if (!(freq > 0.f)) freq = 0.f;      // NaN-safe: a NaN fails (freq > 0) and is caught here
if (freq > maxFreq) freq = maxFreq;
tel.freqHz = freq;
```

**Why this is load-bearing, not cosmetic.** The wrap is a single subtract (`if (phase >= 1.0) phase -= 1.0`),
copied from `LfoCore`. That is only correct while `deltaPhase < 1.0`, i.e. while `freq < sampleRate`.
Measured without the clamp at 44.1 kHz over 200,000 samples: `[MEASURED]`

| `pitchCV` | freq (Hz) | max phase after wrap | wrap broken? | output range (V) | all finite? |
|-----------|-----------|----------------------|--------------|------------------|-------------|
| +2.0 | 1,046.50 | 1.0000 | no | [−4.853, 5.000] | yes |
| +5.0 | 8,372.02 | 1.0000 | no | [−4.853, 5.000] | yes |
| +7.0 | 33,488.08 | 1.0000 | no | [−4.853, 5.000] | yes |
| **+10.0** | **267,904.62** | **1,014,986.94** | **YES** | **[−8,655,011, −39.68]** | **yes** |
| **+14.0** | **4,286,474.00** | **19,239,791.03** | **YES** | **[−164,061,888, −816.71]** | **yes** |

Two consequences the planner must not miss:

1. The failure is **silent to the finiteness invariant** — −8.6 million volts is perfectly `std::isfinite`.
   Only the D-18b magnitude bound catches it. This is the strongest argument for D-18b existing at all.
2. `0.49f * sampleRate` bounds `deltaPhase ≤ 0.49`, comfortably under the 0.5 that would begin aliasing
   the wrap detection, with margin for a future sub-sample sync fraction (Phase 33).

**Where the constant lives.** Namespace scope, plain `constexpr`, following `src/dsp/MathConst.hpp`'s
documented idiom exactly:

```cpp
namespace forge {
// Plain constexpr (internal linkage per TU), NOT `inline constexpr`: inline
// variables are C++17 and the Rack toolchain builds with -std=c++11.
constexpr float kVcoFreqC4 = 261.6256f;         // C4 = 0 V (PITCH-01 reference)
constexpr float kVcoNyquistGuardFrac = 0.49f;   // PROVISIONAL — PITCH-04 (Phase 31) owns the real one
}
```

Do **not** make these in-class `static constexpr` members. Under C++11 the in-class form is a
declaration only; `check_canary.sh`'s whole `[2b/5]` apparatus exists because that construct got
v2.0.0 rejected from the VCV Library.

### Pattern 3: `setSpreadSeed` mirrors `LfoCore` verbatim (D-11)

```cpp
void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
    drift.setSpreadSeed(s0, s1);
    wave.triAsymmetrySpread = drift.triAsymmetrySpread;
    wave.sawCurvatureSpread = drift.sawCurvatureSpread;
    wave.squareDutySpread   = drift.squareDutySpread;
    wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
    wave.bleedSpread        = drift.bleedSpread;
}
```

This is a copy of `src/dsp/LfoCore.hpp:103-112` with the identical five fields. `characterSpread` is
deliberately **not** copied — `LfoCore` does not copy it either (the LFO shell folds it into
`in.character`), and folding it in Phase 30 would silently change what `character = 1.0` means.

### Pattern 4: The interleave independence test (D-17) — the five non-vacuity requirements

A naive interleave test is trivially green, because a correct implementation passes it *and so does an
implementation whose shared state is invisible under the chosen inputs*. Measured requirements:

1. **Two different spread seeds.** With identical seeds, a hypothetical shared `static Waveshape` produces
   identical coefficients and is undetectable.
2. **`character = 1.0`.** Measured detectability of a clobbered `Waveshape` (instance A running with
   instance B's coefficients): `[MEASURED]`

   | character | max abs diff | differing samples |
   |-----------|--------------|-------------------|
   | **0.00** | **0.000000 V** | **0 / 1024 — COMPLETELY UNDETECTABLE** |
   | 0.05 | 0.000520 V | 1024 / 1024 |
   | 0.10 | 0.002074 V | 1024 / 1024 |
   | 0.30 | 0.024821 V | 1024 / 1024 |
   | 0.60 | 0.097162 V | 1024 / 1024 |
   | 1.00 | 0.233187 V | 1024 / 1024 |

   Every spread coefficient in `Waveshape` is gated behind `character >= 0.001f`. **At `character = 0`
   the test proves nothing about shared shaper state.** This is D-10's rationale, measured.
3. **Different per-sample inputs per instance** (e.g. A sweeps `pitchCV`, B holds a different pitch and
   sweeps `morph`). A shared phase accumulator is caught either way, but different inputs also catch a
   shared `freq`, a shared `tel`, or a shared `Waveshape`.
4. **Assert the two solos are distinguishable.** Without this, "interleaved == solo" could be satisfied by
   two identical signals. Measured with the recommended construction: `soloA[i] == soloB[i]` on
   **0 / 1024** samples. `[MEASURED]`
5. **A permanent in-test positive control.** Define a small deliberately-broken stand-in core inside the
   test TU that shares a `static double` phase accumulator, run it through the *same* interleave helper,
   and require it to **fail** the independence property. Measured: 511/512 and 512/512 mismatches with
   different inputs, and 511/512 with identical inputs. `[MEASURED]` This is exactly the house style of
   `check_frozen.sh [3/3]`, `check_includes.sh [6/7]` and `check_canary.sh [4/5]` — a guard validated by
   an observed red on every run, not by never having been anything but green.

Prototype result with the real Phase-30 core: **interleaved vs solo mismatches A = 0/1024, B = 0/1024**,
compared with bit-exact `!=`, not `doctest::Approx`. `[MEASURED]`

### Pattern 5: Second-module registration — the exact additive diff

All four edits below were applied to a scratch copy and **built and linked successfully** against the real
`../Rack-SDK`; `nm` on the resulting `plugin.dylib` shows both `_modelAnalogLFO` and `_modelAnalogVCO`. `[MEASURED]`

```diff
--- src/plugin.hpp
 extern Plugin* pluginInstance;
 extern Model* modelAnalogLFO;
+extern Model* modelAnalogVCO;
```

```diff
--- src/plugin.cpp
 void init(Plugin* p) {
 	pluginInstance = p;
 	p->addModel(modelAnalogLFO);
+	p->addModel(modelAnalogVCO);
 }
```

```diff
--- plugin.json
       "tags": [
         "Low-frequency oscillator",
         "Waveshaper"
       ]
-    }
+    },
+    {
+      "slug": "ForgeAnalogVCO",
+      "name": "Analog VCO",
+      "description": "Audio-rate morphing oscillator with analog character",
+      "tags": [
+        "Voltage-controlled oscillator",
+        "Waveshaper"
+      ]
+    }
   ]
```

```cpp
// src/AnalogVCO.cpp — final line
Model* modelAnalogVCO = createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO");
```

Notes verified against the SDK and the official manifest documentation:
- `Model::slug` "Must be unique. Used for saving patches. **Never change this after releasing your module.**"
  — `Rack-SDK/include/plugin/Model.hpp:35-37`. `[VERIFIED: SDK source]`
- `modules[].slug` and `modules[].name` are the only **required** fields; `tags`, `description`,
  `keywords`, `manualUrl`, `modularGridUrl`, `hidden` are optional. `[CITED: vcvrack.com/manual/Manifest]`
- `"Voltage-controlled oscillator"` is an accepted tag string (a deprecated alias resolving to the
  canonical `Oscillator` tag ID). The shipped LFO entry already uses the alias
  `"Low-frequency oscillator"` and passed VCV library review, so alias tags are accepted by the
  validator. D-03 is safe. `[CITED: vcvrack.com/manual/Manifest]` + `[VERIFIED: shipped plugin.json + issue #929 acceptance]`
- `SOURCES += $(wildcard src/*.cpp)` picks up `src/AnalogVCO.cpp` with **no Makefile edit**; the
  prototype build compiled it automatically. `[MEASURED]`
- `plugin.json` `version` stays `2.0.1` (D-04). `check_frozen.sh`'s banner explicitly documents that
  `plugin.json`, `src/plugin.cpp` and `src/plugin.hpp` are **deliberately not pinned** precisely so
  Phase 30 can edit them without a manifest bump. `[VERIFIED: repo source]`

### Pattern 6: The stub panel SVG — hard requirements

```svg
<svg xmlns="http://www.w3.org/2000/svg" width="91.44mm" height="128.5mm"
     viewBox="0 0 91.44 128.5" version="1.1">
  <rect x="0" y="0" width="91.44" height="128.5" fill="#101014"/>
  <rect x="4" y="6" width="24" height="6" fill="#e85d26"/>
</svg>
```

Verified by running the SDK's own vendored `nanosvg.h` (`nsvgParseFromFile(path, "px", 75.0f)` — Rack's
`SVG_DPI` is `75.f`, `Rack-SDK/include/window/Svg.hpp:16`): `[MEASURED]`

| File | parsed size (px) | HP | shapes |
|------|------------------|-----|--------|
| `res/AnalogLFO.svg` (shipped) | 270.0000 × 379.4291 | **18.00** | 172 |
| the stub above | 270.0000 × 379.4291 | **18.00** | 2 |
| stub with a `<text>` label added | 270.0000 × 379.4291 | 18.00 | **1 — the text was dropped** |
| a `px`-unit variant (270 × 379.53) | 270.0000 × 379.5300 | 18.00 | 1 |

Rules that follow:

- **`<text>` is not rendered and D-06's "text label" cannot be a `<text>` element.** The SDK's vendored
  `nanosvg.h` contains **zero occurrences of the string "text"** and has no text parser at all — its
  element parsers are only `rect`, `circle`, `ellipse`, `line`, `poly(line|gon)` and `path`. A `<text>`
  element is **silently ignored, not an error**: the panel still loads, the label is simply invisible.
  `[VERIFIED: Rack-SDK/dep/include/nanosvg.h]` The official guidance says the same: *"All text objects
  must be converted to paths."* `[CITED: vcvrack.com/manual/Panel]`
  **Planner's call:** draw a crude "VCO" from a few `<rect>`s, or omit the label entirely — Rack's module
  browser shows the name from `plugin.json` regardless, so a label is not needed for identification.
  Do **not** ship a `<text>` element and assume it renders.
- **Units must be mm.** Height is fixed at 128.5 mm; width must be a multiple of 5.08 mm. 18 HP =
  91.44 mm. `[CITED: vcvrack.com/manual/Panel]`
- **Mirror the LFO's header line byte-for-byte** (`width="91.44mm" height="128.5mm" viewBox="0 0 91.44 128.5"`).
  Note 128.5 mm parses to 379.4291 px against the SDK's `RACK_GRID_HEIGHT = 380` — a 0.57 px difference.
  The shipped LFO panel has the identical discrepancy and is live in the VCV Library, so **mirror it, do
  not "fix" it**; changing the height would make Phase 35's art swap non-trivial.
- `ModuleWidget::setPanel()` "sets the size of the ModuleWidget from the panel"
  (`Rack-SDK/include/app/ModuleWidget.hpp:49-51`) — HP comes from the SVG, never from `plugin.json`.
  `[VERIFIED: SDK source]`

### Anti-Patterns to Avoid

- **Asserting on `tel.freqHz` for the pitch test.** D-16 forbids it and is right: `tel.freqHz` is a
  read-back of the number `step()` computed three lines earlier, so the assertion stays green even if the
  phase accumulator ignores `freq` entirely. Measure the **output**.
- **Running the divergence or independence test at `character = 0`.** Measured to be exactly vacuous
  (0.000000 V difference, 0/1024 differing samples). See Pattern 4 and § Finding 5.
- **Using `sweepScenario` alone for the magnitude bound.** It never reaches the overshoot region — see
  § Finding 3.
- **Adding a hard `±5 V` clamp "for safety".** D-13 forbids it explicitly; the overshoot is the behavior
  Phase 34's output stage exists to fix, and hiding it creates work Phase 34 must undo.
- **Editing `src/dsp/Waveshape.hpp` to add a VCO-shaped helper.** ARCHITECTURE Anti-Pattern 1;
  `check_frozen.sh [1/3]` pins its bytes and would hard-fail.
- **Touching `src/AnalogLFO.cpp` at all.** D-08 chose the stock widgets precisely so this file is not in
  the diff.
- **Reformatting `struct VcoCore {` or the `step()` signature line.** Breaks `check_canary.sh [2b/5]`
  (Pattern 1).
- **Brace value-list init of `VcoInputs`.** `VcoInputs in{1.f, 2.f}` is a hard C++11 error (NSDMIs make
  it a non-aggregate). Use `VcoInputs in;` then assign — the harness and canary both document this.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| V/oct → Hz | `std::exp2` / `std::pow(2, v)` | `forge::exp2_taylor5` | D-14. Bit-identical to the Rack SDK polynomial; libm would break the FM path's golden discipline in Phase 31. Its octave part is *exact* (IEEE exponent field), so accuracy is a non-issue (~0.1 cent, non-cumulative). |
| Clamping to [0,1] | `std::clamp` | `forge::clamp` | `std::clamp` is C++17 and is one of the four constructs `check_canary.sh [4/5]` proves the gate rejects. |
| Waveform generation | Any new sine/tri/saw/square/pulse code | `Waveshape::morphedWave` | D-12. The 5-shape crossfade, morph×4 rescale, duty interpolation and bleed ring are written, frozen, and golden-pinned. |
| Per-instance analog variation | A new RNG or a per-sample noise source | `DriftEngine::setSpreadSeed` + the 5-coefficient copy | D-11. Also avoids adding an RNG draw, which would move the LFO goldens (PITFALLS 4b). |
| π | `M_PI` / `<cmath>` macro | `forge::kPi` (already reached via `Waveshape.hpp`) | The Windows direct-g++ CI leg has no `_USE_MATH_DEFINES`. |
| Model registration boilerplate | A hand-written `plugin::Model` subclass | `createModel<M, W>(slug)` | The SDK template builds the factory correctly, including the `assert(mw->module == m)` invariants. |
| Panel size | Hardcoding `box.size` in the widget | `setPanel(createPanel(...))` | The SDK derives the size from the SVG. Hardcoding it desynchronises the widget from the art in Phase 35. |
| Bit-exact float comparison in tests | `doctest::Approx(x).epsilon(0)` | plain `a != b` | `test_vco_harness.cpp:158-160` already documents that `Approx` with `epsilon(0)` still applies a relative-scaling margin and is **not** a true bit-exact comparator. |

**Key insight:** Phase 30 writes ~18 lines of DSP because four frozen headers already contain everything
else. Every line this phase does *not* write is a line that cannot regress the shipped LFO.

---

## Runtime State Inventory

> Included because a **live, library-published** module shares this plugin binary and this phase
> chooses a permanent, user-patch-visible identifier.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | **None.** The VCO has no persisted state in Phase 30 (no `dataToJson`/`dataFromJson`; the seeds are constructor literals). The LFO's own patch JSON (`spreadSeed` hex) is untouched — `src/AnalogLFO.cpp` is not in this phase's diff. Verified by reading `src/AnalogLFO.cpp:1-60` and the D-08 boundary. | None |
| Live service config | **The VCV Library entry (issue #929) and the published `v2.0.1` tag.** These key off `plugin.json`'s `version`, which D-04 holds at `2.0.1`. Phase 30 publishes nothing. | None in Phase 30. Phase 36 owns REL-01. |
| OS-registered state | **None.** No scheduled tasks, services or daemons. Verified: the only build outputs are `plugin.dylib` / `build/`. | None |
| Secrets / env vars | **None.** No `.env`, no SOPS, no CI secrets touched. `.github/workflows/test.yml` uses only `RACK_SDK_VERSION: 2.6.6` (a plain env, not a secret). Verified by reading the workflow. | None |
| Build artifacts / installed packages | **A locally-installed `plugin.dylib` in the Rack user plugins directory** will still contain only the LFO until the operator reinstalls. The user's recorded workflow already includes a stale-install flush step. | Reinstall + flush before the in-Rack check, or the VCO will appear absent and be misdiagnosed as a registration bug. |
| **One-way door** | **The slug `ForgeAnalogVCO`** is written into every user patch containing the module, forever (`Model.hpp:35` — "Never change this after releasing your module"). | D-05 operator surface **before** commit. |

---

## Common Pitfalls

### Pitfall 1: `check_includes.sh [2/7]` false-positives on `dsp/RackCompat.hpp` — CI goes red on the first commit

**What goes wrong:** `make guards` exits 1 with
`FAIL: src/dsp/VcoCore.hpp includes a Rack SDK header — VCO DSP must stay Rack-free (TEST-02)` the moment
`#include "dsp/RackCompat.hpp"` is added to `VcoCore.hpp`. `[MEASURED — reproduced this session]`

**Why it happens:** section `[2/7]`'s detector is

```bash
grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^">]*[Rr]ack[^">]*[>"]' "${h}"
```

The path string `"dsp/RackCompat.hpp"` contains `Rack`. The guard cannot distinguish the Rack **SDK**
from the repo's Rack-free **compatibility shim**. It has never fired because today's `VcoCore.hpp`
includes only `<cstdint>` and `dsp/DriftEngine.hpp`, and `[2/7]` scans the VCO headers **non-transitively**.

**This is an internal contradiction, not just a gap.** `tests/check_canary.sh [5b/5]` explicitly lists
`RackCompat.hpp` among "the D-05 frozen shared headers the VCO is allowed to consume" and passed cleanly
on the same prototype. Two guards in this repository currently disagree about the same include line.

**How to avoid:** see § Guard Script Impact for the exact patch and the no-edit alternative.

**Warning signs:** a green `make test` + green `make strict` with a red `make guards`; the CI
`toolchain-gate` job failing at step 9 (`Include / dependency-direction audit (D-06)`) — or, worse,
being reported `skipped` because an earlier step fail-fasted (a sub-finding Phase 29 already recorded).

### Pitfall 2: Zero-crossing period counting collapses in the narrow-pulse region

**What goes wrong:** at high pitch with `morph` near 1.0, the `+1` region of the pulse is narrower than
one sample, the sampler steps over it, crossings are lost, and the measured frequency lands near half the
true value.

**Measured at 44.1 kHz** (`morph = 1.00` ⇒ duty 0.05; `pulseSamples = duty × sampleRate / freq`): `[MEASURED]`

| `pitchCV` | freq (Hz) | samples/cycle | pulse samples | measured error |
|-----------|-----------|---------------|---------------|----------------|
| +2.0 | 1,046.50 | 42.14 | 2.11 | +0.0002 % |
| +2.5 | 1,479.98 | 29.80 | 1.49 | −0.0003 % |
| +3.0 | 2,093.00 | 21.07 | 1.05 | +0.0029 % |
| **+3.5** | **2,959.96** | **14.90** | **0.74** | **−24.53 %** |
| **+4.0** | **4,186.01** | **10.54** | **0.53** | **−46.89 %** |

**Why it happens:** `morphedWave`'s `pulseDuty = 0.50 − 0.45 × min(pulseFrac, 1)` reaches 0.05 at
`morph = 1.0`. That is a genuine feature of the shape, not a bug — and Phase 30 has no anti-aliasing by
design (Phase 32 owns CORE-02), so the sampler is expected to miss it.

**How to avoid:** constrain the D-16 test grid so `duty × (sampleRate / freq) ≥ 2` samples. At
`morph = 1.0` and 44.1 kHz that caps `freq` at `sampleRate / 40 = 1102 Hz`, i.e. `pitchCV ≤ +2.07`.
The recommended grid in § Measured Constants stays inside this envelope at every point, with the worst
case at `pitchCV = +2, morph = 1.00, 44.1 kHz` = 2.11 pulse samples.

**Warning signs:** a pitch assertion that fails only at 44.1 kHz and only at high `morph`; a measured
frequency that is a clean fraction (≈ ½) of the expected one.

### Pitfall 3: `sweepScenario` never reaches the >5 V overshoot the bound is about

**What goes wrong:** a magnitude-bound test driven only by `VcoBlockDriver::sweepScenario` measures a
maximum of **exactly 5.0000 V** at every sample rate and every block length tested, so a bound of 6.0 V
is never approached and the "expected overshoot" of D-13 is never observed. `[MEASURED]`

**Why it happens:** `sweepScenario` sets `morph = t` and `character = 1 − t` — the two are
**anti-correlated**. The output peak occurs at `morph = 0` *with* `character = 1`, a combination the
sweep only passes through in its first few samples, before the phase accumulator has reached the peak
phase.

**How to avoid:** run the D-18b bound over **both** `sweepScenario` **and** a fixed worst-case scenario
(`morph = 0.0`, `character = 1.0`). Measured maxima over a 1-second block: `[MEASURED]`

| scenario | max &#124;out&#124; |
|----------|---------------------|
| `sweepScenario` | 5.0000 V |
| morph 0.00 / character 1.00 | **5.5180 V** |
| morph 0.00 / character 0.50 | 5.1355 V |
| morph 1.00 / character 1.00 | 5.0140 V |
| morph 0.50 / character 1.00 | 5.0000 V |
| morph 0.00 / character 0.00 (pure sine) | 5.0000 V |

**Warning signs:** a bound test that would still pass with the bound set to 5.001 V.

### Pitfall 4: Reintroducing the v2.0.0 ODR / C++17 rejection class in a brand-new TU

**What goes wrong:** `src/AnalogVCO.cpp` is a fresh translation unit and the exact place VCO modules
grow tables (waveform-name arrays, sync-mode labels, semitone tables). An in-class
`static constexpr T arr[]` that is indexed at runtime is a **declaration only** under C++11; MinGW's
linker fails with `undefined reference`. That class got v2.0.0 rejected from the VCV Library.

**What this phase can actually hit.** D-07 limits the shell to four controls, so **no table is required**
and none should be added. The two scalars this phase introduces must be namespace-scope `constexpr`
(Pattern 2). `Waveshape::morphedWave` already contains `float shapes[5]` — an *automatic* array, not a
static, already shipped, not a hazard.

**How to avoid:** namespace-scope `constexpr` for scalars (the `MathConst.hpp` idiom); if any array ever
becomes necessary, use a file-scope `static const`/`constexpr` at namespace scope, never in-class.

**Verified:** the prototype passes `make strict`
(`AnalogLFO.cpp + AnalogVCO.cpp + plugin.cpp + vco_compile_canary.cpp`, `-std=c++11 -pedantic-errors`)
and builds and links against Rack-SDK. `[MEASURED]`

**Warning signs — and the standing rule.** Phase 29 proved that the **entire local gate** returns exit 0
on code that cannot link (`make test`, `make strict`, `make guards`, `check_canary.sh` all green on the
deliberately broken commit `e117cff`), and that `-fsyntax-only` gates are blind on the Ubuntu runner too.
Only the CI MinGW **link** step catches this class. The standing rule from STATE.md applies unchanged:
**no tag or resubmission on local evidence alone.**

### Pitfall 5: Finiteness does not catch a runaway accumulator

`std::isfinite(-8655011.0f)` is `true`. The measured no-clamp failure in Pattern 2 produces outputs six
orders of magnitude out of range while every finiteness assertion stays green. D-18b's magnitude bound is
the only invariant in the suite that catches it. Do not treat finiteness and the bound as redundant.

### Pitfall 6: The pitch test is *too* accurate and gets mistaken for TEST-02

With sub-sample interpolated crossings the measured worst-case error inside the safe envelope is
**0.0078 %** — better than one cent (0.0578 %). A reader could reasonably conclude the tracking gate is
already met. It is not: TEST-02 requires `< 1 cent` **across the pitch range with coarse, fine and FM
summing**, none of which exist until Phase 31. D-16 requires the test be **explicitly labelled "NOT the
TEST-02 tracking gate"** in its case name and banner. Do not soften that label because the numbers look
good.

---

## Code Examples

### Code Example 1: the verified `VcoCore::step()` body

This exact prototype produced every measurement in this document. It passes `make strict`, builds and
links against Rack-SDK 2.6.6, and leaves `make test` at 66/67 with the TOMBSTONE as the only failure.
`[MEASURED]`

```cpp
// src/dsp/VcoCore.hpp — additions only; VcoInputs and Telemetry are UNCHANGED.

#include <cstdint>

#include "dsp/DriftEngine.hpp"   // forge::DriftEngine
#include "dsp/RackCompat.hpp"    // forge::exp2_taylor5, forge::clamp   <-- see Guard Script Impact
#include "dsp/Waveshape.hpp"     // forge::Waveshape::morphedWave (FROZEN — call, never edit)

namespace forge {

// Namespace-scope plain constexpr — the src/dsp/MathConst.hpp idiom. NOT
// `inline constexpr` (C++17), NOT an in-class static constexpr (C++11 declaration-only).
constexpr float kVcoFreqC4 = 261.6256f;         // C4 = 0 V, the standard VCV reference
constexpr float kVcoNyquistGuardFrac = 0.49f;   // PROVISIONAL safety — PITCH-04 (Phase 31) owns the real one

struct VcoCore {                                 // <-- keep on ONE line (check_canary.sh [2b/5])
	DriftEngine drift;

	double phase = 0.0;      // double precision, mirroring LfoCore (PITFALLS 2.2)
	Waveshape wave;          // per-instance: this is what CORE-03 asserts

	// ... Telemetry unchanged ...

	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	float step(const VcoInputs& in) {             // <-- keep on ONE line (check_canary.sh [2b/5])
		tel.lastSampleTime = in.sampleTime;
		tel.lastSampleRate = in.sampleRate;
		++tel.stepCount;

		float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);
		const float maxFreq = kVcoNyquistGuardFrac * in.sampleRate;
		if (!(freq > 0.f)) freq = 0.f;            // NaN-safe: NaN fails (freq > 0)
		if (freq > maxFreq) freq = maxFreq;
		tel.freqHz = freq;

		double deltaPhase = (double)freq * (double)in.sampleTime;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;           // safe ONLY because deltaPhase <= 0.49

		const float p = (float)phase;
		const float morph = clamp(in.morph, 0.f, 1.f);
		const float character = clamp(in.character, 0.f, 1.f);
		const float sample = wave.morphedWave(p, morph, character, 0.f);  // bleedLfo = 0 (D-12)
		tel.displayPhase = p;
		return 5.f * sample;                      // NO conditioning (D-13)
	}
};

} // namespace forge
```

### Code Example 2: the minimum viable Rack shell (`src/AnalogVCO.cpp`)

Verified to compile at `-std=c++11 -pedantic-errors` and to link into `plugin.dylib`. `[MEASURED]`

```cpp
#include "plugin.hpp"
#include "dsp/VcoCore.hpp"

struct AnalogVCO : Module {
	enum ParamId  { MORPH_PARAM, CHARACTER_PARAM, PARAMS_LEN };
	enum InputId  { VOCT_INPUT, INPUTS_LEN };
	enum OutputId { OUTPUT, OUTPUTS_LEN };
	enum LightId  { LIGHTS_LEN };

	forge::VcoCore core;

	AnalogVCO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");          // mirrors AnalogLFO.cpp:199
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");  // mirrors AnalogLFO.cpp:200
		configInput(VOCT_INPUT, "V/Oct");
		configOutput(OUTPUT, "Audio");
		core.seed(0x1234ULL, 0x5678ULL);
		core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);          // never (0,0) — Xoroshiro fixed point
	}

	void process(const ProcessArgs& args) override {
		forge::VcoInputs in;                        // never a brace value list (C++11 non-aggregate)
		in.pitchCV    = inputs[VOCT_INPUT].getVoltage();
		in.morph      = params[MORPH_PARAM].getValue();
		in.character  = params[CHARACTER_PARAM].getValue();
		in.sampleTime = args.sampleTime;
		in.sampleRate = args.sampleRate;
		outputs[OUTPUT].setVoltage(core.step(in));
	}
};

struct AnalogVCOWidget : ModuleWidget {
	AnalogVCOWidget(AnalogVCO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogVCO.svg")));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48f, 40.f)), module, AnalogVCO::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.96f, 40.f)), module, AnalogVCO::CHARACTER_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48f, 100.f)), module, AnalogVCO::VOCT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60.96f, 100.f)), module, AnalogVCO::OUTPUT));
	}
};

Model* modelAnalogVCO = createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO");
```

> Seeding note: hardcoded literals are correct for Phase 30 — there is no `dataToJson` yet, and the
> harness supplies its own seeds. Phase 34/35 will move to OS-entropy seeding forwarded from the shell
> with `PatchParse` persistence (ARCHITECTURE Anti-Pattern 3). Whatever the values, they must never be
> `(0, 0)` (Xoroshiro fixed point → `std::normal_distribution` infinite loop → Rack hangs on patch load).

### Code Example 3: the D-16 period estimator (use this, not count/duration)

```cpp
// Rising zero crossings with linear sub-sample interpolation, first-to-last.
// Worst measured error inside the safe envelope: 0.0078 %.
static double estimateFreqRising(const std::vector<float>& o, double sr, int* nUp) {
	double first = -1.0, last = -1.0; int count = 0;
	for (size_t i = 1; i < o.size(); ++i) {
		if (o[i - 1] < 0.f && o[i] >= 0.f) {
			const double frac = (double)(-o[i - 1]) / ((double)o[i] - (double)o[i - 1]);
			const double t = ((double)(i - 1) + frac) / sr;
			if (count == 0) first = t;
			last = t;
			++count;
		}
	}
	*nUp = count;
	return (count < 2) ? -1.0 : (count - 1) / (last - first);
}
```

**Do not use `crossings / 2 / duration`.** That estimator carries a `0.5 / cyclesInWindow` quantization
error: measured **−2.15 %** at `pitchCV = −2` over a 250 ms window — which would force a tolerance so
loose the test stops being evidence. `[MEASURED]`

### Code Example 4: the D-17 interleave test skeleton

```cpp
// Solo baselines through the harness (keeps the seeding discipline in one place).
forge::VcoBlockDriver sa(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
forge::VcoBlockDriver sb(sr, 0xC0FFEEULL, 0xBADF00DULL, 0xDEADBEEFULL, 0xCAFEF00DULL);
auto soloA = sa.run(n, inA);   // inA sweeps pitchCV, character = 1.f
auto soloB = sb.run(n, inB);   // inB holds a DIFFERENT pitch and sweeps morph, character = 1.f

// Non-vacuity precondition — without this the assertion below is trivially satisfiable.
// Measured: soloA[i] == soloB[i] on 0 / 1024 samples.
int same = 0; for (int i = 0; i < n; ++i) if (soloA[i] == soloB[i]) ++same;
CHECK(same < n / 10);

// Interleaved, sample by sample, through freshly-seeded drivers.
forge::VcoBlockDriver ia(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
forge::VcoBlockDriver ib(sr, 0xC0FFEEULL, 0xBADF00DULL, 0xDEADBEEFULL, 0xCAFEF00DULL);
const float dt = (float)(1.0 / sr);
for (int i = 0; i < n; ++i) {
	forge::VcoInputs a = inA(i); a.sampleTime = dt; a.sampleRate = (float)sr;
	CHECK(ia.core.step(a) == soloA[i]);      // bit-exact, NOT doctest::Approx
	forge::VcoInputs b = inB(i); b.sampleTime = dt; b.sampleRate = (float)sr;
	CHECK(ib.core.step(b) == soloB[i]);
}
```

Plus the permanent positive control described in Pattern 4, item 5.

---

## Measured Constants — the four discretionary tolerances, resolved

| Constant | **Recommended value** | Margin over measurement | Derivation |
|----------|----------------------|-------------------------|------------|
| **D-18b loose magnitude bound** | `\|out\| <= 6.0f` V | 8.1 % above the hard ceiling; ~10⁶× below the runaway failure | Hard analytic ceiling **5.55 V**, derived + confirmed (below) |
| **D-16 pitch tolerance** | `1 %` relative | ~128× the worst measured error (0.0078 %) | Sub-sample rising-crossing estimator inside the safe envelope |
| **D-18a divergence threshold** | `maxAbsDiff > 0.01f` V **and** `>90 %` of samples differ | ≥13× below the smallest divergence measured across 5 seed pairs | Measured 0.138–0.480 V at `character = 1.0` |
| **D-14 Nyquist fraction** | `0.49f * sampleRate` | `deltaPhase ≤ 0.49` vs the 1.0 that breaks the wrap | Wrap-safety measurement in Pattern 2 |

### Derivation of the ±5.55 V ceiling

For `character ≥ 0.001`, the sine path is `f(s) = s + 0.08·T₃(s) + 0.03·T₂(s)` with `s = sin(2πφ)`,
i.e. `f(s) = 0.32s³ + 0.06s² + 0.76s − 0.03`. Its derivative `0.96s² + 0.12s + 0.76` is strictly positive,
so `f` is monotone on `[−1, 1]` with range **`[−1.05, +1.11]`**. Triangle, saw, square and pulse are each
bounded by 1 in magnitude. The morph crossfade is a linear interpolation between two shapes, so it cannot
exceed the larger. The bleed step is `result ← (result + b·bleedSignal) / (1 + b)` with `b ≥ 0` and
`|bleedSignal| ≤ 1.11`, which is a convex combination and cannot raise the maximum. Therefore
**`|morphedWave| ≤ 1.11` exactly**, and `|out| = 5·|morphedWave| ≤ 5.55 V`.

Exhaustive numerical confirmation over 401 × 201 × 2001 = **161 million** `(morph, character, phase)`
points at five spread configurations: `[MEASURED]`

| spread configuration | max &#124;morphedWave&#124; | ×5 | argmax |
|----------------------|------------------------------|-----|--------|
| zero spread | 1.105620 | 5.5281 V | morph 0.0000, character 1.000, phase 0.2499 |
| **−4σ all** | **1.109999** | **5.5500 V** | morph 0.0000, character 1.000, phase 0.2499 |
| +4σ all | 1.097962 | 5.4898 V | same |
| mixed ±4σ | 1.097532 | 5.4877 V | same |
| `VcoBlockDriver` default spread seed | 1.103605 | 5.5180 V | same |

The `−4σ` case reaches the analytic ceiling because a sufficiently negative `bleedSpread` drives
`effectiveBleed = max(0, 0.04 + bleedSpread)` to zero, removing the bleed normalization. **6.0 V is a
real bound**, not a round number.

### The zero-crossing structure is sound

Across a **401 × 101 grid of `(morph, character)` = 40,501 points**, sampled at 20,000 points per cycle
with the driver's default spread, **every single point produced exactly 2 sign changes per cycle** — zero
exceptions. Maximum per-cycle DC was **0.8995** (at `morph = 1.00`, `character = 0.00`, the 5 %-duty
pulse), and even there the count is exactly 2. `[MEASURED]`

So the failure mode is **not** spurious or DC-suppressed crossings in the continuous waveform; it is
purely **sampling loss in the narrow-pulse region** (Pitfall 2). That is a much narrower risk than the
phase brief anticipated, and it is fully characterized above.

### Recommended D-16 test grid (all points verified safe)

`pitchCV ∈ {−2, −1, 0, +1, +2}` × `morph ∈ {0.00, 0.25, 0.50, 0.75, 1.00}` ×
`character ∈ {0.0, 0.5, 1.0}` × `sampleRate ∈ {44100, 48000, 96000}`, 250 ms window.

Tightest point: `pitchCV = +2, morph = 1.00, 44.1 kHz` → 2.11 pulse samples, measured error +0.0002 %.
Worst error anywhere in this grid: **0.0078 %**. `[MEASURED]`
Avoid `pitchCV ≥ +3` combined with `morph > 0.9`.

### Recommended D-18a divergence measurements

`[MEASURED]` — fixed drift seed `(0xC0FFEE, 0xBADF00D)`, spread seeds `(0x9E3779B9, 0x7F4A7C15)` vs
`(0xDEADBEEF, 0xCAFEF00D)`:

| scenario | 44.1 kHz | 48 kHz | 96 kHz | differing samples |
|----------|----------|--------|--------|-------------------|
| `sweepScenario`, n = 0.05 s | 0.134023 V | 0.133872 V | 0.139847 V | 2196/2205 |
| fixed `morph 0.25 / character 1.0 / pitchCV 0`, n = 2048 | 0.233229 V | 0.233235 V | 0.233187 V | 2048/2048 |
| **`character = 0.0` (any morph)** | **0.000000 V** | — | — | **0 / 2048** |

Across five different seed pairs at `morph ∈ {0.25, 0.50}`, `character = 1.0`, the max-abs difference
ranged **0.137976 – 0.480358 V**; the smallest observed was 0.1380 V. A threshold of **0.01 V** therefore
carries ≥13× margin against the least-divergent pair measured, while being ~13,000× above float noise.

> **`character = 0.0` produces exactly zero divergence.** Any divergence test written at
> `character = 0` is not merely weak — it is guaranteed to fail. Use `character = 1.0`.

---

## Guard Script Impact

Verified by applying a full Phase-30 prototype (DSP + `AnalogVCO.cpp` + registration + stub SVG) to an
isolated scratch copy and running each guard. `[MEASURED]`

| Guard section | Verdict | Detail |
|---------------|---------|--------|
| `check_frozen.sh [1/3]` | ✅ **PASS, no edit** | 15/15 entries OK; completeness sweep covers `src/dsp/*.hpp` + `src/AnalogLFO.cpp` only, and skips `src/dsp/Vco*.hpp`. `src/AnalogVCO.cpp` is **not** swept. `plugin.json`, `src/plugin.cpp`, `src/plugin.hpp` are **deliberately unpinned** — the script's own banner says so, naming Phase 30. `FROZEN_EXPECTED_ENTRIES` needs **no bump**. |
| `check_frozen.sh [2/3]`, `[3/3]` | ✅ PASS, no edit | 6/6 goldens OK; negative control fires. |
| `check_includes.sh [1/7]` | ✅ **PASS, no edit** | 29 LFO-side root files, 29 opened across the transitive closure, zero VCO includes. `src/AnalogVCO.cpp` is **already** in `VCO_SIDE_ALLOW` (line 220 — Phase 29 pre-registered it). `extern Model* modelAnalogVCO;` in `plugin.hpp` is not an `#include` line and does not match the detector. Confirmed by regex reading **and** by the passing run. |
| **`check_includes.sh [2/7]`** | ❌ **FAILS — needs an edit** | `FAIL: src/dsp/VcoCore.hpp includes a Rack SDK header`. False positive on `#include "dsp/RackCompat.hpp"` (see Pitfall 1). |
| `check_includes.sh [3/7]` | ✅ PASS, no edit | Both new includes begin with `dsp/`. |
| `check_includes.sh [4/7]` | ✅ PASS, no edit | Still exactly one `struct Inputs`, in `LfoCore.hpp`. `VcoInputs` is untouched. |
| `check_includes.sh [5/7]`, `[6/7]`, `[7/7]` | ✅ PASS, no edit | No hasher under `src/`; both negative controls fire; wiring intact. |
| `check_canary.sh [1/5]`, `[2/5]` | ✅ PASS, no edit | Probe declared, defined, and emitted at −O3. |
| **`check_canary.sh [2b/5]`** | ✅ **PASS — but constrains the source shape** | *"all 8 VcoInputs DSP field(s) stay runtime-live through step() at −O3"*. Requires `struct VcoCore {` and `float step(const VcoInputs& in) {` to each stay on one line (Pattern 1). |
| `check_canary.sh [3/5]`, `[4/5]` | ✅ PASS, no edit | C++11 clean; all four C++17-isms rejected for the expected reasons. |
| `check_canary.sh [5/5]` | ✅ PASS, no edit | `dsp/VcoCore.hpp` still included by the canary. |
| **`check_canary.sh [5b/5]`** | ✅ **PASS — and directly contradicts `[2/7]`** | *"every header in the VCO seam is Vco*-named or an allowed frozen shared header"* — its allow-list is exactly `DriftEngine.hpp \| MathConst.hpp \| RackCompat.hpp \| Waveshape.hpp`. |
| `make strict` | ✅ PASS | Now compiles `AnalogLFO.cpp + AnalogVCO.cpp + plugin.cpp + vco_compile_canary.cpp`. |
| `make` (plugin build vs Rack-SDK) | ✅ PASS | `plugin.dylib` links; `nm` exports `_modelAnalogLFO` **and** `_modelAnalogVCO`. |
| `make test` | ⚠️ 66/67 — **TOMBSTONE is the only failure**, exactly as D-15 intends | All 6 LFO goldens green; harness cases 5 (determinism) and 6 (finiteness) green **with real DSP** — D-19 re-evidencing needs no harness change. |
| `.github/workflows/test.yml` | ✅ **No edit needed** | The MinGW leg loops `for f in src/*.cpp` and links every object into `plugin.dll`; `AnalogVCO.cpp` rides along automatically. `make guards` is already a CI step. |
| `Makefile` | ✅ **No edit needed** | `SOURCES`, `TEST_SOURCES`, `TEST_HEADERS`, `strict` and `GUARD_SCRIPTS` are all wildcards or already complete. |

### The required patch — `tests/check_includes.sh [2/7]`

**Option A (recommended).** Keep the explicit `#include "dsp/RackCompat.hpp"` in `VcoCore.hpp` and
exempt exactly that path, with the reasoning written in place:

```diff
 	for h in "${VCO_HEADERS[@]}"; do
 		rel="${h#${ROOT}/}"
-		rack_hits="$(grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^">]*[Rr]ack[^">]*[>"]' "${h}" || true)"
+		# dsp/RackCompat.hpp is the repo's own RACK-FREE compatibility shim, not a
+		# Rack SDK header: it is one of the four D-05 frozen shared headers the VCO
+		# is explicitly allowed to consume (see check_canary.sh [5b/5]'s allow-list),
+		# its bytes are pinned by check_frozen.sh, and it contains zero Rack includes.
+		# Its FILENAME merely contains "Rack", which this detector cannot distinguish
+		# from a genuine SDK include. Without this exemption, D-14's mandated use of
+		# forge::exp2_taylor5 makes this section a guaranteed false positive.
+		# Any OTHER [Rr]ack-named include still fails, including <rack.hpp>.
+		rack_hits="$(grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^">]*[Rr]ack[^">]*[>"]' "${h}" \
+			| grep -vE '#[[:space:]]*include[[:space:]]*"dsp/RackCompat\.hpp"' || true)"
 		if [[ -n "${rack_hits}" ]]; then
```

The exemption is **exact-path**, so a vendored `dsp/rack.hpp`, a `"dsp/RackSDK.hpp"`, or any
`<rack...>` angle include still fails. Because this weakens a standing guard, it belongs on the
**same operator surface as the D-05 registration diff**, not in a silent commit.

*Recommended companion:* add a negative control alongside `[6/7]` proving `[2/7]` still rejects a
synthetic VCO header carrying `#include <rack.hpp>` — otherwise the exemption is unvalidated, which is
precisely the posture this repository has repeatedly rejected.

**Option B (no guard edit).** Omit the explicit include and rely on `dsp/DriftEngine.hpp` transitively
supplying `RackCompat.hpp`. This **compiles today** and `make guards` stays green with zero edits. It is
recorded here because it is a legitimate escape hatch, but it is **not recommended**: it makes
`VcoCore.hpp` silently dependent on another header's include list, and it contradicts the repo's own
practice — `LfoCore.hpp:29` includes `RackCompat.hpp` explicitly even though `DriftEngine.hpp` would
provide it.

**Note for Phase 32.** `MorphBlep.hpp` will hit the identical trap the moment it needs `forge::clamp` or
`exp2_taylor5`. Fixing `[2/7]` now removes a landmine from a later phase too.

### Finding 7: keep `src/vco_compile_canary.cpp` — the reasoning is stronger than CONTEXT.md's

CONTEXT.md recommends keeping it and cites the D-08 permanence rule plus the proven CI bite. Two
**additional, measured** reasons make retirement actively harmful:

1. **Retiring it deletes four guard sections outright.** `check_canary.sh` sections `[1/5]`, `[2/5]`,
   `[2b/5]`, `[5/5]` and `[5b/5]` all begin with `if [[ ! -f "${CANARY}" ]]; then note_fail ...`. Without
   the canary, the entire script must be rewritten or removed — and removing it trips
   `check_includes.sh [7/7]`'s wiring audit and requires editing both `GUARD_SCRIPTS` and the CI workflow.
   The constant-fold guard `[2b/5]` in particular has **no** substitute: it perturbs `VcoCore.hpp` and
   compiles *the canary* against it.
2. **`AnalogVCO.cpp` does not provide equivalent coverage.** The canary feeds **runtime-derived values
   into all 8 `VcoInputs` DSP fields** (`pitchCV, coarse, fine, fmVolts, fmAtten, morph, character, drift`)
   — that is what makes `[2b/5]` report *"all 8 fields stay runtime-live at −O3"*. Phase 30's
   `AnalogVCO.cpp::process()` sets only **3** of them (`pitchCV`, `morph`, `character`); the other five
   keep their NSDMI literals, which `-O3` can constant-propagate. So swapping the canary for
   `AnalogVCO.cpp` would silently **reduce** ODR/constant-fold coverage from 8 fields to 3 — reopening
   exactly the hole `[2b/5]` was written to close, and in the phases (31, 33, 34) that will actually add
   `coarse`/`fine`/`fmVolts` DSP.

**Verdict: KEEP, and record reason 2 in STATE.md** — it converts "keep it for tradition" into a
measured coverage argument that survives the next reviewer who asks the same question.

---

## State of the Art

| Old (Phase 29) | Current (Phase 30) | Impact |
|----------------|--------------------|--------|
| `VcoCore::step()` returns `0.f`; TOMBSTONE asserts silence | Real naive oscillator; TOMBSTONE **inverted** to assert non-silence and non-constancy | The Phase 29→30 transition is one readable diff line; a future refactor cannot silently revert to a stub |
| TEST-01 determinism + finiteness recorded **green-but-weak** | Both load-bearing under real DSP (verified: green with the prototype, over the varying sweep) | D-19 debt closes; `sweepScenario` needed no change |
| `plugin.json` has one `modules[]` entry | Two entries; version still `2.0.1` (D-04) | The working tree never claims a release that was not cut |
| No `src/AnalogVCO.cpp`; canary is the sole VCO TU under `src/` | Both exist; the canary keeps 8-field runtime coverage the shell does not provide | Finding 7 |
| `check_includes.sh [2/7]` never exercised on a VCO header with a `dsp/` dependency | Fires as a false positive on `RackCompat.hpp` | A standing guard needs an exact-path exemption + a negative control |

**Deprecated / superseded within this phase:**
- `kVcoNyquistGuardFrac = 0.49f` is **provisional**. PITCH-04 (Phase 31) replaces the policy on the same
  surface. Mark it as such in the source comment so Phase 31 does not have to rediscover the intent.
- The stub `res/AnalogVCO.svg` is replaced wholesale in Phase 35. Only its **filename and dimensions**
  are durable.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `"Voltage-controlled oscillator"` will pass the VCV Library manifest validator when #929 is updated in Phase 36 | Pattern 5 | LOW. The tag is documented as accepted, and the shipped LFO already uses a sibling alias that passed review. Worst case is a Phase-36 tag rename, which is cheap (tags are not slugs). |
| A2 | Rack renders a panel whose SVG height parses to 379.4291 px against `RACK_GRID_HEIGHT = 380` without visual artefacts | Pattern 6 | VERY LOW. The shipped LFO panel has the identical value and is live in the library. Confirmed visually only by that precedent, not by running Rack in this session. |
| A3 | The `0.49f` Nyquist fraction will not conflict with PITCH-04's eventual design | Pattern 2 | LOW. PITCH-04's own wording is "frequency is clamped just below Nyquist" — the same surface and the same semantics. Phase 31 changes a constant, not a structure. Flagged for the planner to state explicitly in the source comment. |
| A4 | Hardcoded construction seeds in `AnalogVCO.cpp` are acceptable for Phase 30 | Code Example 2 | LOW. No patch persistence exists yet, so no user-visible state depends on them. Phase 34/35 replaces them with shell-forwarded OS entropy. |
| A5 | The measured tolerances hold on the Linux/MinGW CI toolchains, not just Apple clang | Measured Constants | MEDIUM. All measurements ran under Apple clang 16 at `-O2 -ffp-contract=off` (the `make test` flags). The recommended margins (128× on pitch, 13× on divergence, 8 % on the bound) are far wider than any plausible cross-libm ULP difference, and the shapes' drift-off goldens already agree across the three CI toolchains. Still: **the first CI run is the confirmation**, per the standing "no local evidence alone" rule. |

---

## Open Questions

1. **Should `check_includes.sh [2/7]` gain a negative control alongside the exemption?**
   - What we know: this repository's standing posture is that an unvalidated guard is worse than no guard
     (`check_includes.sh [6/7]`'s banner, `check_frozen.sh [3/3]`, `check_canary.sh [4/5]`).
   - What's unclear: whether Phase 30 should absorb the cost, or whether it is Phase 32's problem when
     `MorphBlep.hpp` lands.
   - Recommendation: **add it in Phase 30**, in the same commit as the exemption. It is ~10 lines
     (synthetic VCO header with `#include <rack.hpp>`, require a hit) and the exemption is otherwise
     unvalidated at the exact moment it is introduced.

2. **Where does the D-17 positive control live so it cannot be mistaken for real code?**
   - What we know: it must be a deliberately-broken core with shared static state, which is precisely the
     construct CORE-03 forbids.
   - Recommendation: define it inside an **anonymous namespace in `tests/test_vco_core.cpp`** with a name
     that makes intent unmistakable (e.g. `struct DeliberatelyBrokenSharedStateCore`), and a banner
     explaining it is a permanent control. It never enters `src/`, so `check_includes.sh` and the C++11
     gates never see it.

3. **Does the in-Rack check need a stale-install flush?**
   - What we know: the operator's recorded build/install workflow already includes one.
   - Recommendation: make it an explicit step in the UAT instructions. A VCO that "does not appear" after
     a successful build is far more likely a stale `plugin.dylib` than a registration bug, and
     misdiagnosing it would send the operator hunting a phantom `addModel` problem.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| C++ compiler | `make test` (C++17), `make strict` (C++11), plugin build | ✓ | Apple clang 16.0.0 (clang-1600.0.26.6) | — |
| GNU make | all targets | ✓ | 3.81 (the Makefile is explicitly 3.81-compatible) | — |
| VCV Rack SDK | plugin build, `make strict` | ✓ | present at `../Rack-SDK` (relative path is load-bearing — the repo path contains a space, which breaks an absolute `RACK_DIR`) | — |
| `nm` | `check_canary.sh [2/5]`, `[2b/5]` | ✓ | system binutils | none — the guard hard-fails without it |
| `shasum` | `check_frozen.sh` | ✓ | macOS Perl shasum | `sha256sum`, then `openssl` |
| bash | all three guard scripts | ✓ | system bash | — |
| doctest | `make test` | ✓ | 2.4.11, vendored at `tests/doctest.h` | — |
| Rack (the application) | in-Rack UAT only | ✓ (operator machine) | — | headless tests cover everything except the visual/audible check |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** none.
**New dependencies introduced by this phase:** none.

---

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | doctest 2.4.11, vendored at `tests/doctest.h` |
| Config file | none — `tests/main.cpp` (`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`) + `Makefile` `TEST_CXXFLAGS` |
| Quick run command | `make test` |
| Full suite command | `make test && make strict && make guards` |
| Filtered runs | `./build-test/test -tc="vco core*"` (new), `-tc="vco harness*"`, `-tc="golden*"` |
| **Measured baseline (phase start, 2026-07-28)** | **67 cases / 67 passed / 0 failed / 2,615,121 assertions**; `make strict` PASS; `make guards` PASS |
| Measured gate latency | ~4.3 s incremental for the full local gate (Phase 29 measurement, re-confirmed) |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CORE-01 | Swept block is **not** all-zero and **not** constant (D-15 tombstone inversion) | unit | `./build-test/test -tc="vco harness*"` | ✅ `tests/test_vco_harness.cpp` (case 7 rewritten) |
| CORE-01 | Output period matches `261.6256 · 2^pitchCV` within 1 % across the safe grid, measured on the **output** (D-16) — labelled **NOT the TEST-02 tracking gate** | unit | `./build-test/test -tc="vco core: naive pitch*"` | ❌ Wave 0 — `tests/test_vco_core.cpp` |
| CORE-01 | `\|out\| <= 6.0 V` over `sweepScenario` **and** the fixed `morph 0 / character 1` worst case (D-18b) | unit | `./build-test/test -tc="vco core: output magnitude*"` | ❌ Wave 0 — `tests/test_vco_core.cpp` |
| CORE-01 | Output finite (no NaN/Inf) under real DSP — **D-19 re-evidenced** | unit | `./build-test/test -tc="vco harness: output is finite*"` | ✅ exists; banner rewritten (verified green with real DSP) |
| CORE-01 | Same seeds → bit-identical block under real DSP — **D-19 re-evidenced** | unit | `./build-test/test -tc="vco harness: seam determinism*"` | ✅ exists; banner rewritten (verified green with real DSP) |
| CORE-01 | Different spread seed → `maxAbsDiff > 0.01 V` and >90 % of samples differ, at `character = 1.0` (D-18a) | unit | `./build-test/test -tc="vco core: spread seed divergence*"` | ❌ Wave 0 — `tests/test_vco_core.cpp` |
| CORE-03 | Two instances interleaved sample-by-sample reproduce their solo blocks bit-exactly, with the 5 non-vacuity preconditions (D-17) | unit | `./build-test/test -tc="vco core: two-instance independence*"` | ❌ Wave 0 — `tests/test_vco_core.cpp` |
| CORE-03 | **Positive control**: a deliberately-shared static accumulator **fails** the same independence check | unit | `./build-test/test -tc="vco core: independence positive control*"` | ❌ Wave 0 — `tests/test_vco_core.cpp` |
| PANEL-03 | Plugin builds and links with both models; `nm` exports `modelAnalogLFO` **and** `modelAnalogVCO` | structural | `make && nm -gU plugin.dylib \| grep modelAnalog` | ✅ verified on the prototype |
| PANEL-03 | `plugin.json` is valid JSON with exactly two `modules[]` entries and the LFO entry byte-unchanged | structural | `python3 -m json.tool plugin.json` + `git diff plugin.json` reviewed at the D-05 operator surface | ✅ property of the diff |
| PANEL-03 | `res/AnalogVCO.svg` parses to 18.00 HP with ≥1 shape | structural | verified via nanosvg in research; in-Rack UAT is the operational check | ✅ verified on the prototype stub |
| PANEL-03 | LFO goldens byte-identical (standing guardrail) | regression | `./build-test/test -tc="golden*"` + `make guards` | ✅ verified green on the prototype |
| all | VCO headers + new shell compile at `-std=c++11 -pedantic-errors` | compile gate | `make strict` | ✅ verified PASS on the prototype |
| all | Guard suite clean | property/hash/static | `make guards` | ⚠️ **requires the `[2/7]` patch** — see § Guard Script Impact |
| all | MinGW compile **+ link** vs libRack | **CI-only** link gate | `toolchain-gate` job on push | ✅ job exists; `AnalogVCO.cpp` rides the existing `src/*.cpp` glob |

### Sampling Rate

- **Per task commit:** `make test` (~0.5 s incremental). Add `make strict` on any commit touching `src/`.
- **Per wave merge:** `make test && make strict && make guards`.
- **Phase gate:** full local gate green **and** both CI jobs green on the pushed commit — specifically the
  `toolchain-gate` MinGW link step, which is the only gate that can see the link-class defect. Phase 29
  measured the entire local gate returning exit 0 on code that could not link; that finding stands.
- **Max feedback latency:** ~5 s locally; CI on every push.

### Wave 0 Gaps

- [ ] `tests/test_vco_core.cpp` — covers CORE-01 (pitch, magnitude, divergence) and CORE-03 (independence + positive control)
- [ ] `src/AnalogVCO.cpp` — covers PANEL-03 and provides the in-Rack surface
- [ ] `res/AnalogVCO.svg` — 91.44 mm × 128.5 mm stub, **no `<text>` element**
- [ ] `tests/check_includes.sh [2/7]` exemption + its negative control — **blocking**: `make guards` and CI fail without it
- [ ] Framework install: **none needed** — doctest vendored, compiler present, Rack SDK present

### Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| The VCO appears in Rack's module browser as "Analog VCO" and makes sound | PANEL-03, CORE-01 | Requires the Rack application and a human ear; the crude aliased timbre is the expected result, not a defect | Build, install, **flush the stale install**, restart Rack, add the module, patch a keyboard/V-Oct source to V/OCT and OUT to an audio module, sweep MORPH and CHARACTER, confirm every control audibly does something (D-07) |
| The LFO is visually and audibly unchanged | milestone guardrail | Visual/audible | In the same session, add the Analog LFO, confirm the panel and behavior are unchanged |
| The D-05 registration diff | PANEL-03 | Operator judgement on a one-way door (the slug) | Present the `plugin.cpp` / `plugin.hpp` / `plugin.json` diff **plus the `check_includes.sh [2/7]` guard patch** before commit |

---

## Security Domain

This is an offline audio DSP plugin: no network, no authentication, no session, no persistence, no user
accounts, no secrets. The applicable surface is **hostile input handling on the audio thread**.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| **V5 Input Validation** | **yes** | `forge::clamp` on `morph`/`character`; the NaN-safe `if (!(freq > 0.f))` guard; the Nyquist clamp; explicit non-`(0,0)` seeds |
| V6 Cryptography | no | The SHA-256 in `tests/` is an integrity **tripwire**, not a security control, and `check_includes.sh [5/7]` mechanically keeps it out of `src/` |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Hostile/garbage V/OCT voltage → exponent overflow → NaN/Inf → stuck or silent voice | Denial of Service | `if (!(freq > 0.f)) freq = 0.f;` then the Nyquist clamp (Pattern 2). Measured: without it, a large `pitchCV` produces −8.6 M V while remaining `isfinite`. PITCH-04 hardens this properly in Phase 31 by clamping the summed pitch **before** the `exp2`. |
| `(0,0)` Xoroshiro seed → `std::normal_distribution` rejection loop never terminates → **Rack hangs on patch load** | Denial of Service | Non-zero construction seeds (Code Example 2); `test_vco_harness.cpp` case 4 is the standing hang guard. Phase 34/35 must validate any *deserialized* seed the same way `AnalogLFO`'s BUG-04 fix does. |
| Unbounded output voltage damaging downstream modules | Tampering | Out of scope by decision (D-13) — OUT-01..03 in Phase 34. Phase 30's measured ceiling is 5.55 V, well within Rack's ±12 V norms. |
| VCO code silently entering the shipped LFO's build graph | Tampering (supply chain) | `check_includes.sh [1/7]` transitive-closure audit with two negative controls — verified passing with `AnalogVCO.cpp` present |
| Slopsquatted / hallucinated dependency | Tampering (supply chain) | N/A — this phase adds zero external packages |

---

## Reproduction Commands

Every `[MEASURED]` claim comes from one of these. All were run against an **isolated scratch copy**; the
real working tree was verified clean before and after.

```bash
# Baseline (real repo, unmodified)
make test        # 67 cases / 67 passed / 2,615,121 assertions
make strict      # strict C++11 gate: PASS
make guards      # guard suite: PASS

# Prototype experiment (scratch copy only)
#   1. rsync the repo (excluding .git, build*, .planning) to a scratch dir
#   2. apply the Code Example 1 + Code Example 2 + registration + stub SVG changes
#   3. symlink ../Rack-SDK next to the scratch copy (relative RACK_DIR is load-bearing)
make guards      # check_frozen PASS; check_includes FAILS at [2/7]; check_canary PASS
make strict      # PASS (now compiling AnalogVCO.cpp)
make             # links plugin.dylib
nm -gU plugin.dylib | grep -i modelAnalog     # _modelAnalogLFO and _modelAnalogVCO
make test        # 66/67 — TOMBSTONE is the sole failure (D-15 working as designed)

# Numerical probes (compiled with the same flags as `make test`)
c++ -std=c++17 -O2 -ffp-contract=off -Isrc  probe.cpp  -o probe   # magnitude sweep, sign changes, DC
c++ -std=c++17 -O2 -ffp-contract=off -Isrc  probe2.cpp -o probe2  # estimators, pulse loss, wrap safety
c++ -std=c++17 -O2 -ffp-contract=off -Isrc -Itests probe3.cpp -o probe3  # sweep bounds, divergence, interleave
c++ -std=c++17 -O2 -ffp-contract=off -Isrc -Itests probe4.cpp -o probe4  # fixed worst-case magnitudes

# nanosvg panel check (SDK's own vendored parser, Rack's own DPI)
cc -O1 -I../Rack-SDK/dep/include svgtest.c -o svgtest -lm
./svgtest res/AnalogLFO.svg      # 270.0000 x 379.4291 px = 18.00 HP, 172 shapes
```

---

## Sources

### Primary (HIGH confidence)

- **Repository source, read directly:** `src/dsp/VcoCore.hpp`, `LfoCore.hpp`, `Waveshape.hpp`,
  `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp`; `src/AnalogLFO.cpp` (head + registration tail);
  `src/plugin.cpp`, `src/plugin.hpp`, `src/vco_compile_canary.cpp`; `plugin.json`; `Makefile`;
  `.github/workflows/test.yml`; `tests/VcoBlockDriver.hpp`, `tests/test_vco_harness.cpp`,
  `tests/check_frozen.sh`, `tests/check_includes.sh`, `tests/check_canary.sh`; `res/AnalogLFO.svg`.
- **VCV Rack SDK headers, read directly:** `include/plugin/Model.hpp` (slug permanence),
  `include/helpers.hpp` (`createModel`, `createPanel`), `include/app/ModuleWidget.hpp` (`setPanel` sizing),
  `include/window/Svg.hpp` (`SVG_DPI = 75`, `mm2px`), `include/app/common.hpp` (`RACK_GRID_*`),
  `include/engine/Module.hpp` (`config`/`configParam`/`configInput`/`configOutput`),
  `dep/include/nanosvg.h` (**zero `text` occurrences**; parser inventory).
- **Direct measurement in this session:** four numerical probes, the prototype build/link, the guard-suite
  runs, and a nanosvg parse harness. See § Reproduction Commands.
- **Phase artifacts:** `29-CONTEXT.md`, `29-VALIDATION.md`, `.planning/STATE.md` § Accumulated Context,
  `.planning/REQUIREMENTS.md`, `.planning/research/ARCHITECTURE.md`, `.planning/research/PITFALLS.md`.

### Secondary (MEDIUM confidence)

- <https://vcvrack.com/manual/Manifest> — `modules[]` required/optional fields, version format, tag
  validity and alias status.
- <https://vcvrack.com/manual/Panel> — 128.5 mm height, 5.08 mm HP multiple, mm units, "all text objects
  must be converted to paths".

### Tertiary (LOW confidence)

- None. Context7 MCP was unavailable in this runtime; the seam-selected provider was replaced by direct
  SDK source reads (a strictly more authoritative source for API mechanics) plus the official manual for
  the manifest/panel policy that is not encoded in headers.

---

## Metadata

**Confidence breakdown:**

- **Standard stack — HIGH.** Zero new dependencies; every component read directly from repo or SDK source
  and exercised in a building, linking prototype.
- **Architecture — HIGH.** The `step()` body, the shell, and the four-file registration diff were all
  built, linked, and run. Both model symbols confirmed exported.
- **Tolerance constants — HIGH.** Derived analytically **and** confirmed numerically over 161 M sample
  points, five spread configurations, five seed pairs and three sample rates. Cross-toolchain
  confirmation is deferred to the first CI run (assumption A5), but every recommended margin is ≥8×.
- **Guard-script impact — HIGH.** Each verdict comes from an observed run against a real prototype, not
  from reading the scripts alone. The `[2/7]` failure was reproduced.
- **Panel/SVG — HIGH.** Verified through the SDK's own vendored nanosvg at Rack's own DPI, cross-checked
  against the shipped LFO panel and the official manual.
- **Manifest tags — MEDIUM.** Documentation plus the shipped LFO's alias precedent; final confirmation is
  Phase 36's library submission.

**Research date:** 2026-07-28
**Valid until:** 2026-08-27 (30 days — a stable, frozen, in-repo stack; the only external moving part is
the VCV Library's manifest validator, which Phase 36 re-checks anyway)
