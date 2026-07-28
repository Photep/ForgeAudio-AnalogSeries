# Phase 30: VcoCore Skeleton & Module Registration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-28
**Phase:** 30-vcocore-skeleton-module-registration
**Areas discussed:** Module identity & manifest, Stub shell scope (P30 vs P35), Naive DSP scope & seed divergence, Test evidence after the tombstone

---

## Module identity & manifest

### Q1 — Permanent VCO module slug

| Option | Description | Selected |
|--------|-------------|----------|
| ForgeAnalogVCO | Direct mirror of the shipped `ForgeAnalogLFO`; series-consistent | ✓ |
| AnalogVCO | Shorter, drops brand prefix; breaks symmetry with the shipped LFO slug | |
| ForgeAnalogOsc | Generic "Osc" leaves naming room for future VCO variants; loses browser recognizability | |

**User's choice:** ForgeAnalogVCO
**Notes:** Treated explicitly as a one-way door — written into every user patch containing the module, forever.

### Q2 — Display name

Not put to a vote. Stated as an assumption ("Analog VCO", matching "Analog LFO") and left uncontested.

### Q3 — plugin.json tags

| Option | Description | Selected |
|--------|-------------|----------|
| VCO + Waveshaper | `["Voltage-controlled oscillator", "Waveshaper"]` — mirror of the LFO's two tags with oscillator class swapped | ✓ |
| VCO only | Maximally precise; loses the Waveshaper cross-listing | |
| VCO + Waveshaper + Hardware clone | Signals analog modeling; VCV reserves the tag for actual clones — reviewer-objection risk on #929 | |
| VCO + Waveshaper + Polyphonic | Pre-declares CORE-03 polyphony-readiness; would be a user-facing lie until v2.1 | |

**User's choice:** VCO + Waveshaper

### Q4 — When does plugin.json version bump

| Option | Description | Selected |
|--------|-------------|----------|
| Not until Phase 36 | Phase 30 adds only the `modules[]` entry; version stays 2.0.1; Phase 36 owns REL-01 | ✓ |
| Bump to 2.1.0 now | Signals "new module added" immediately; leaves an unreleased version sitting in-tree for six phases | |
| Bump to a -dev suffix | Makes WIP unmistakable; VCV's validator expects plain MAJOR.MINOR.PATCH — suffix risks failing library tooling | |

**User's choice:** Not until Phase 36

---

## Stub shell scope (P30 vs P35)

### Q1 — Phase 30 stub panel

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal throwaway res/AnalogVCO.svg | Plain dark rect at final 18HP width; establishes real filename + HP so Phase 35 is an art swap | ✓ |
| Reuse res/AnalogLFO.svg | No new asset; renders LFO labels on VCO controls — misleading, and couples to a shipped asset | |
| Full 18HP Forge Noir panel now | Pulls PANEL-01/02 forward; commits layout before the DSP informs it | |

**User's choice:** Minimal throwaway res/AnalogVCO.svg

### Q2 — How many controls the shell declares

| Option | Description | Selected |
|--------|-------------|----------|
| Only what the P30 DSP consumes | Every visible control does something; honest in-Rack check | ✓ |
| Declare full PANEL-02 enum, wire what's live | Stable IDs from day one; dead controls during every intermediate check | |
| Declare full enum, widgets only for live | Middle path; ID-stability benefit is near-zero when nothing has shipped | |

**User's choice:** Only what the P30 DSP consumes
**Notes:** Framed with the observation that nothing ships until Phase 36, so no user patches exist and param/input ID churn is currently free.

### Q3 — Knob/jack widgets

| Option | Description | Selected |
|--------|-------------|----------|
| Stock Rack widgets, zero LFO touch | Phase 30 diff never touches `src/AnalogLFO.cpp` | ✓ (as a consequence) |
| Extract Forge components to shared header now | Reuses the Forge look; edits a shipped LFO file, needs operator surface + visual confirmation | |
| Duplicate component structs into AnalogVCO.cpp | Forge look without an LFO edit; guaranteed cleanup debt | |

**User's choice (free text):** *"I am happy with the current jacks - but the knobs are my least favourite part of the current design. I'd like to take an opportunity to update them and then backport those new knob styles back into the LFO module"*
**Notes:** Recognized as a real design intent but outside Phase 30's boundary — the stub panel is a deliberately ugly throwaway, so there would be nothing to judge new knobs on. Surfaced the mechanical fact that knob art is shared (`res/components/ForgeKnob*.svg`, `ForgeTrimpot*.svg`), so a single asset edit updates both modules and the "backport" is automatic. Routed to a deferred idea; Phase 30 proceeds with stock Rack widgets.

### Q4 — Where the knob redesign lands

| Option | Description | Selected |
|--------|-------------|----------|
| Its own phase before 35 | Dedicated iteration budget; VCO panel then built around settled knobs | ✓ |
| Fold into Phase 35 | Keeps all panel work together; Phase 35 already carries PANEL-01/02 + DISP-01/02/03 | |
| Backlog it | Decide placement later; risks Phase 35 building around disliked knobs | |

**User's choice:** Its own phase before 35

---

## Naive DSP scope & seed divergence

### Q1 — How "different seed diverges" becomes true

| Option | Description | Selected |
|--------|-------------|----------|
| Component spread only, no OU drift stepping | `setSpreadSeed` copies spread coefficients into `Waveshape`; static per-instance analog variation, no per-sample RNG | ✓ |
| Step the DriftEngine too | Seed drives live modulation now; pre-empts Phase 34's audition-gated DRIFT-03 authority decision | |
| Leave output seed-independent | Honest about scope; leaves a stated roadmap success criterion unmet at phase close | |

**User's choice:** Component spread only
**Notes:** Framed by the criterion-4 tension — `morphedWave` is a pure function, so divergence must come from somewhere seed-derived. Follow-up finding surfaced immediately after: every spread coefficient in `Waveshape` is gated behind `character >= 0.001f`, which makes a CHARACTER knob a derived requirement of this choice rather than a Phase-34 pull-forward.

### Q2 — Output scaling

| Option | Description | Selected |
|--------|-------------|----------|
| ×5 to ±5V, no conditioning | Rack-normal levels, zero output stage; overshoot at high character expected and audible | ✓ |
| ×5 with a hard clamp | Strict ±5V invariant now; Phase 34 must undo it, and it hides the overshoot | |
| Raw ±1 | Purest deferral; inaudibly quiet, weakens criterion 3's "sounds crudely in Rack" | |

**User's choice:** ×5 to ±5V, no conditioning

### Q3 — How much pitch chain lands now

| Option | Description | Selected |
|--------|-------------|----------|
| Reference + exp2 + double phase + Nyquist guard | C4=0V→261.6256 Hz via `exp2_taylor5`, double phase, clamp as safety not scope | ✓ |
| Same minus the clamp | Cleanest scope line; harness must avoid high-CV or accept garbage above Nyquist | |
| Full Phase 31 pitch chain now | Pulls an entire phase forward; Phase 31's value is its <1-cent measurement discipline | |

**User's choice:** Reference + exp2 + double phase + Nyquist guard

---

## Test evidence after the tombstone

### Q1 — What replaces the tombstone

| Option | Description | Selected |
|--------|-------------|----------|
| An inverted non-silence case | Same slot, opposite assertion; transition visible in one diff line, guards against silent reversion to a stub | ✓ |
| Just delete it | Determinism/finiteness become non-vacuous anyway; loses the explicit signal | |
| Full waveform-shape assertion | Stronger, but a golden fixture in disguise — TEST-05 is Phase 36, and Phase 32 will change the waveform | |

**User's choice:** An inverted non-silence case

### Q2 — How pitch is checked in Phase 30

| Option | Description | Selected |
|--------|-------------|----------|
| Loose zero-crossing period count on the output | Proves the accumulator actually advances at the computed rate; labelled "not the tracking gate" | ✓ |
| Assert on tel.freqHz telemetry only | Simplest; only re-reads what `step()` just computed — the vacuous-coverage trap | |
| No pitch assertion in Phase 30 | Clean scope line; leaves a "pitch-accurate" phase with no pitch evidence | |

**User's choice:** Loose zero-crossing period count on the output

### Q3 — How CORE-03 (no static/global mutable voice state) is enforced

| Option | Description | Selected |
|--------|-------------|----------|
| Two-instance independence test | Interleaved stepping of two seeded cores must match their solo runs; tests the actual polyphony property | ✓ |
| A grep/nm guard in CI | Mechanical and cheap; pattern-matches source text, misses indirectly shared state | |
| Both | Belt-and-braces, matching Phase 29 posture; second guard to maintain | |
| Code-review convention only | Lowest cost; Phase 29's finding was that unenforced green is not evidence | |

**User's choice:** Two-instance independence test

### Q4 — Anything beyond re-evidencing the weak pair

| Option | Description | Selected |
|--------|-------------|----------|
| Add a loose bound + a divergence case | Five real invariants replace four weak ones; divergence directly proves criterion 4 | ✓ |
| Just re-evidence the existing pair | No test would then assert criterion 4's divergence half | |
| Add divergence only, no bound | Covers criterion 4; loses the runaway/sign-error catch | |

**User's choice:** Add a loose bound + a divergence case

---

## Claude's Discretion

- Stub-panel geometry, control placement, param ranges.
- Tolerance constants: zero-crossing period-count percentage, the loose magnitude bound, the "measurably different" divergence threshold.
- Whether the Nyquist guard clamps `freqHz` or the phase increment, and where the constant lives.
- Whether new invariants extend `tests/test_vco_harness.cpp` or land in a new `tests/test_vco_core.cpp`.
- Whether the guard scripts need updating now that `src/AnalogVCO.cpp` exists.
- Whether the Phase-29 compile canary retires (recommendation recorded in CONTEXT.md: keep it).

## Deferred Ideas

- **Knob redesign + LFO backport** — new knob styles for the VCO, applied back to the shipped LFO via the shared `res/components/` assets. Jacks stay as-is. Routed to its own dedicated phase before Phase 35; insert via `/gsd-phase`.
