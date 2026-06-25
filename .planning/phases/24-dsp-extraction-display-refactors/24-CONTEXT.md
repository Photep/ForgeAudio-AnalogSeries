# Phase 24: DSP Extraction + Display Refactors - Context

**Gathered:** 2026-06-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Land the **display/thread cleanups and final shell hygiene** on top of the already-extracted DSP core, all verified behavior-preserving.

**CRITICAL — scope is smaller than the ROADMAP's literal text.** Phase 22 (CONTEXT D-02/D-04) already landed the full DSP core extraction AND the `process()` shell-thinning:
- `process()` already delegates the whole per-sample chain to `core.step()` (`src/AnalogLFO.cpp:315`).
- The DSP core lives in `src/dsp/*.hpp` (`LfoCore`, `Waveshape`, `ClockTracker`, `DriftEngine`, `RatioTable`, `Swing`, `RackCompat`, `PatchParse`).
- The `ouLayers[0].state` → explicit `bleedLfo` param lift (ROADMAP criterion 1 / P22 D-05) is **done** (`AnalogLFO.cpp:184`).

So ROADMAP Phase 24 success criteria **#1 (extraction/thinning) and #2 (DSP output identical) are already satisfied** by Phase 22's green golden harness. The **remaining, real work** is:

- **CLEAN-05** — move display-buffer regeneration off the audio thread.
- **CLEAN-04** — make display animations frame-rate-independent (clamped wall-clock dt).
- **CLEAN-03** — ratio/BPM pills fade out symmetrically with the SYNC badge on clock disconnect.
- **CLEAN-01** — remove dead code (`drawZeroCrossing`, `scanlineImage`).
- **CLEAN-02** — resolve the unreachable `isStill`.

**NOT in this phase:** any new DSP behavior (LFO is feature-frozen); IP/font-history purge (→ Phase 25); VCV packaging/manifest (→ Phase 26).

</domain>

<decisions>
## Implementation Decisions

### CLEAN-05 — Display buffer off the audio thread
- **D-01:** **Hybrid dirty-flag.** The audio thread (`process()`) keeps the cheap trigger detection (phase-wrap via `core.phase`, the ~30fps param-change throttle) but only **sets an atomic "dirty" flag**. The GUI widget's `step()` reads the flag and runs the expensive `DISPLAY_SAMPLES` fill (today's `updateDisplayBuffer`, `AnalogLFO.cpp:167-187`, currently called from the audio thread at `:345`). Lowest-risk: proven trigger logic is untouched, only the heavy loop crosses the thread boundary, and it stays behind the existing lock-free double-buffer + `displayReadIdx` atomics.
- **D-02:** **Snapshot at trigger.** When the audio thread sets the flag, it snapshots the exact inputs the preview shape depends on — `morph`, `character`, effective `swingFrac` (the `t.isClocked ? swingFrac : 0.5f` gate), and the live `bleedLfo` (= `core.drift.ouLayers[0].state`) — into a small lock-free struct that the GUI fill consumes. The rendered preview shape must stay **bit-identical** to today's audio-thread behavior, including the live drift state captured at the trigger instant (NOT re-read live at paint time).

### CLEAN-04 — Frame-rate-independent animations
- **D-03:** **Convert ALL time-based animations to clamped wall-clock dt**, including the ANIM-02-locked badge decay. Replace the per-frame `flashIntensity *= 0.92f` (`AnalogLFO.cpp:429`) with its continuous-time equivalent `pow(0.92, dt*60)` so it is **mathematically identical at 60fps** yet frame-rate-independent. Same treatment for breathe (`:388`), blink (`:392`), scanline scroll (`:396`), and the 200ms fade timers (`fadeSpeed`, `:402`). The ANIM-02 lock is respected via feel-preservation, not by leaving the decay frame-paced.
- **D-04:** **dt source = `APP->window->getLastFrameDuration()`** (idiomatic for a Widget `step()`), **clamped to ~33ms (~2 nominal frames, 1/30s)** to cap pathological large dt after a window stall / tab-out. The mandated CLEAN-04 pathological-large-dt test asserts animations advance only by the clamped amount.

### CLEAN-03 — Pill fade-out symmetry
- **D-05:** **Widget-side cache.** Root cause (CODE-REVIEW #10): `displayRatioIndex` → -1 immediately on disconnect and the draw gates early-return on the sentinel (`ratioIdx >= 0` at `:891`; `drawBpmStack` on the sentinel period), so ratio + BPM pills pop off instantly while the SYNC badge fades over 200ms. Fix entirely in the draw layer: cache the last valid `ratioIdx` + period and keep drawing the cached content while `ratioFadeAlpha`/`bpmFadeAlpha` exceed the threshold; replace the `ratioIdx >= 0` early-return with an alpha gate. **No atomic/DSP changes** (the "hold the atomics" alternative was rejected — it would alter what the audio thread publishes and risk consumers that read -1 as "free-running"). Hz and Swing overlays are already correct (their content stays valid during fade) — leave them.

### Verification / Behavior-preservation gate
- **D-06:** **Headless display-buffer determinism test + manual in-Rack UAT.** Automate what's deterministic: a headless test proving the thread-moved fill produces a **byte-identical buffer** for the same snapshot inputs regardless of which thread fills it (guards CLEAN-05). Plus the **mandated** CLEAN-04 pathological-dt test. Then a **manual in-Rack UAT** to confirm overall visual feel (animations, fades, pill symmetry) before closing — subjective/visual behavior is human-gated, matching the project's established "test what's deterministic, human-gate the visual" pattern.

### CLEAN-01 / CLEAN-02 — Clear-cut (planner's discretion)
- **D-07:** Remove dead code `drawZeroCrossing` (`AnalogLFO.cpp:529`) and `scanlineImage` (`:373`) — CLEAN-01. Resolve the unreachable `isStill` (`:974`) — CLEAN-02. These are mechanical; no design decision needed.

### Claude's Discretion
- Exact dirty-flag/snapshot struct layout and memory-ordering; precise clamp constant expression; whether the headless buffer-determinism test lives in the existing doctest/BlockDriver harness or a new test file; test naming/organization; how `isStill` is resolved (remove vs. wire to a reachable condition). All implementation details for the researcher/planner.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Findings being fixed (primary spec for this phase)
- `CODE-REVIEW-FINDINGS.md` §8/§9/§10/§11/§12 — the cleanup items: dead code (#8 → CLEAN-01), unreachable `isStill` (#9 → CLEAN-02), pill fade-out symmetry (#10 → CLEAN-03, root cause + suggested cache fix at lines 163-168), frame-rate-dependent animations (#11 → CLEAN-04, names `getLastFrameDuration()` and the 144Hz failure mode), display buffer on the audio thread (#12 → CLEAN-05).

### Requirements & roadmap
- `.planning/REQUIREMENTS.md` — CLEAN-01..05 and TEST-02. NOTE: TEST-02 (full extraction) effectively landed in Phase 22 per P22 D-04; confirm the requirement-ownership table rather than re-doing extraction.
- `.planning/ROADMAP.md` §"Phase 24" — goal, the 5 success criteria, and the dependency chain. The "open decision at phase start: RNG strategy" listed there is **already resolved** (see Carried Forward) — do NOT re-open it.

### Prior phase context (extraction already done — read to avoid redoing it)
- `.planning/phases/22-test-harness-foundation/22-CONTEXT.md` — D-02 (full `LfoCore` extraction), D-04 (Phase 24 re-scope to cleanups only), D-05 (`bleedLfo` lift), D-07 (Xoroshiro RNG decided + implemented), D-08 (golden-output regression that already proves DSP behavior preserved).

### Source under refactor
- `src/AnalogLFO.cpp` (1,144 lines) — key sites: `updateDisplayBuffer` (:167), audio-thread fill call (:345), display-atomic publish block (:316-330), widget `step()` animations (:386-429), fade-target logic (:399-419), ratio/BPM pill draw gates (:891 + `drawBpmStack`), `drawZeroCrossing` (:529), `scanlineImage` (:373), `isStill` (:974).
- `src/dsp/*.hpp` — the extracted core (read-only here; `core.wave.morphedWave` and `core.drift.ouLayers[0].state` are the display-fill dependencies).

### Project framing
- `.planning/PROJECT.md` — v1.4 Tempered: LFO feature-frozen, release hardening only.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **Lock-free double-buffer** (`displayBuffers[2]` + `displayReadIdx`, `AnalogLFO.cpp:101/168/186`): already the audio→display bridge; CLEAN-05 reuses it — only the *writer* moves from audio thread to GUI thread.
- **Published display atomics** (`displayPhase`, `displaySwingFraction`, `displayClockState`, `displayRatioIndex`, `displaySmoothedPeriod`, etc., `:316-330`): already in place; the dirty-flag + snapshot struct join this existing atomic surface.
- **Unified `fadeSpeed` fade-target loop** (`:399-419`): all overlay alphas already share the 200ms ramp — CLEAN-03 only needs the draw layer to stop early-returning on the ratio/period sentinel.

### Established Patterns
- Audio thread publishes telemetry via relaxed atomics; GUI `step()`/`draw()` consumes them. The dirty-flag + snapshot fits this exactly.
- "Test what's deterministic, human-gate the visual/audio" — v1.3 carried manual-only Nyquist validation for inherently visual behaviors; D-06 follows the same split.
- Build/install: relative `../Rack-SDK`, stale-install flush (project memory `vcv_build_install_workflow.md`). `make test` stays additive/Rack-free.

### Integration Points
- New dirty-flag atomic + snapshot struct on the module, set in `process()`, consumed in `WaveformDisplay::step()`.
- `APP->window->getLastFrameDuration()` is GUI-thread-only — used inside the widget `step()`, never the audio thread.

</code_context>

<specifics>
## Specific Ideas

- Strong behavior-preservation intent throughout: the moved display fill must produce a **byte-identical buffer** (D-02/D-06), and the badge decay must **feel identical at 60fps** (`pow(0.92, dt*60)`, D-03). Treat "looks and feels exactly the same, just frame-rate-independent and off the audio thread" as the hard success bar.
- The badge `0.92` factor and the gate-tuned flash/glow constants are ANIM-02-locked — preserve via mathematical equivalence, never by re-tuning.

</specifics>

<deferred>
## Deferred Ideas

- **Carried forward (already resolved — do NOT re-ask):** RNG strategy (P22 D-07: `forge::Xoroshiro128Plus` re-implemented, bit-identical drift); full DSP core extraction + `process()` thinning + `bleedLfo` lift (P22 D-02/D-04/D-05). The ROADMAP's "open decision: RNG strategy" for Phase 24 is stale.
- **IP/font git-history purge, LICENSE/NOTICES** → Phase 25 (must run while repo is PRIVATE).
- **VCV manifest validation, `.vcvplugin` packaging, CI validation** → Phase 26.
- **`swingIndex` GUI→audio non-atomic write** — pre-existing tech debt carried from v1.3 (common VCV menu-param pattern); not in this phase's scope unless it surfaces while touching display code.

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 24-dsp-extraction-display-refactors*
*Context gathered: 2026-06-26*
