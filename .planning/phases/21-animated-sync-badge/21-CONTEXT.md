# Phase 21: Animated SYNC Badge - Context

**Gathered:** 2026-06-13
**Status:** Ready for planning

<domain>
## Phase Boundary

Make the existing SYNC badge in the waveform display visually pulse on each
incoming clock edge, giving immediate feedback that the module is receiving and
responding to clock. Scope is the SYNC badge **only** — no other display
element (BPM stack, ratio pill, border glow) changes.

Locked upstream:
- **ANIM-01** — SYNC badge flashes on each clock edge with a bright pulse.
- **ANIM-02** — Flash uses exponential decay (~0.92× per frame) back to steady
  state (≈200ms). The decay mechanism is fixed by requirement — not a decision
  to revisit.
- Badge position/size are final from Phase 20.1 (18HP panel).

</domain>

<decisions>
## Implementation Decisions

### Flash Visual Treatment
- **D-01:** The flash reads as "brighter than steady-state" via **a color shift
  toward white-hot PLUS a glow bloom**, applied together at the peak. Steady
  state already renders at full ember alpha (1.0), so brightness cannot come
  from alpha alone. At the peak the badge text/glow shifts from ember
  (`#e85d26`, rgb 0.91/0.365/0.149) toward a hot white-yellow, and the existing
  glow text pass blooms (larger blur radius + stronger glow alpha). Both decay
  back to the steady ember look via the locked ~0.92×/frame envelope. Rationale:
  reads like a spark striking hot metal — on-theme for "Forge" — and is the only
  way to clearly exceed the full-alpha ember steady state.

### Flash Trigger State
- **D-02:** The per-edge flash fires in the **LOCKED state only**, not during
  ACQUIRING. ACQUIRING already owns the dedicated 2 Hz blink (`blinkPhase`,
  semantics = "searching for the clock"). Layering a per-edge flash there would
  create two competing animations on the same badge. Clean separation of meaning:
  **2 Hz blink = acquiring/searching, per-edge flash = locked & responding to
  each beat.** Edges continue to be detected every beat while LOCKED, so the
  trigger source already exists.

### Flash Intensity
- **D-03:** Peak intensity is **clearly visible / punchy** — success criterion #1
  requires the flash be distinct from steady-state at a glance. The peak pushes
  well toward white-hot; the exponential decay keeps it from feeling harsh. The
  exact peak magnitude (color hue at peak + glow boost factor) is a single-constant
  tune to be finalized at the phase acceptance gate.

### Claude's Discretion
- All three areas above were delegated ("you decide"); the decisions reflect the
  builder's recommendation and are now locked. Fine-tuning of exact numeric
  constants (peak white-point, glow blur radius, brightness multiplier) is left
  to implementation + the acceptance gate.
- **Data-flow mechanism (planner's call):** the flash needs a lock-free signal
  from the audio thread to the `WaveformDisplay` widget. Suggested approach,
  mirroring the existing `display*` atomics: an `std::atomic<int>` edge counter
  incremented on each LOCKED clock edge (at the `clockTrigger.process` site,
  AnalogLFO.cpp:446), which the widget's `step()` watches for changes to retrigger
  a decaying flash-intensity envelope. Exact field names/shape are the planner's
  to decide.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` §Animation — ANIM-01 (flash on each clock edge),
  ANIM-02 (exponential decay ~0.92×/frame back to steady state). Both LOCKED.

### Roadmap
- `.planning/ROADMAP.md` → Phase 21 — goal + 2 success criteria (visible bright
  flash distinct from steady-state; smooth ~200ms decay, no abrupt transitions).

### Prior phase context (badge + display lineage)
- `.planning/phases/20.1-panel-redesign-18hp-fresh-layout/20.1-CONTEXT.md` — final
  18HP badge position/sizing the flash must respect.
- `.planning/phases/20-display-layout-crt-aesthetic/20-CONTEXT.md` — display
  three-column layout, CRT treatment, ember palette decisions the flash must
  visually match.

No external ADRs/specs beyond the above — requirements fully captured in the
decisions above.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `WaveformDisplay::drawPillText()` (src/AnalogLFO.cpp:1161) — renders the badge:
  ember-tinted rounded-rect fill + ember stroke + a **glow text pass**
  (`nvgFontBlur(2.0)`, alpha×0.3) + a **sharp text pass** (alpha 1.0). The flash
  treatment (color shift + glow bloom) hooks into these two text passes.
- `WaveformDisplay::step()` (src/AnalogLFO.cpp:868) — already runs per-frame
  animation timers (breathePhase, blinkPhase, fade alphas) with a `1/(0.2*60)`
  200ms fade convention. The flash-decay envelope advances here.
- Existing `display*` `std::atomic` fields (src/AnalogLFO.cpp:110-162) — the
  established lock-free DSP→widget publishing pattern to mirror for the edge signal.

### Established Patterns
- Ember color constant `nvgRGBAf(0.91f, 0.365f, 0.149f, …)` is used throughout —
  the flash's white-hot peak is a lerp away from this toward white.
- Clock state machine: `enum ClockState { FREE, ACQUIRING, LOCKED }`
  (src/AnalogLFO.cpp:40); `displayClockState` atomic published to the widget.
  Widget already branches on ACQUIRING for the 2 Hz blink (line 1373) — the flash
  branches on LOCKED.
- Edge detection: `dsp::SchmittTrigger clockTrigger` fires at
  `clockTrigger.process(clkVoltage, 0.1f, 1.0f)` (line 446) — the single point
  where a LOCKED-state edge counter would increment.

### Integration Points
- DSP `process()` edge-detection block (~line 446) → increment edge signal when
  `clockState == LOCKED`.
- `WaveformDisplay::step()` → detect edge-signal change, set flash envelope to peak.
- SYNC badge draw block (line 1369-1380) → apply flash envelope to color + glow.

</code_context>

<specifics>
## Specific Ideas

- Visual metaphor: a spark/strike on hot metal — ember heating to white-hot and
  cooling back. This drove the "color shift + bloom" choice (D-01).
- The flash must remain readable but in keeping with the restrained Forge Noir
  aesthetic — punchy peak, smooth (non-strobe) decay.

</specifics>

<deferred>
## Deferred Ideas

### Reviewed Todos (not folded)
The following pending todos keyword-matched Phase 21 but are unrelated to badge
animation and remain in the backlog as separate capabilities:
- **Separate display pills from waveform visualiser** — display architecture
  refactor, not animation. Its own future work.
- **Surge-style modulation routing system** — large new feature; own phase/milestone.
- **Pulse width modulation** — already delivered by Phase 18 (PWM DSP Extension);
  candidate for closing, unrelated to this phase.

</deferred>

---

*Phase: 21-animated-sync-badge*
*Context gathered: 2026-06-13*
