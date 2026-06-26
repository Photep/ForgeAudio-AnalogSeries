# Phase 24: DSP Extraction + Display Refactors - Research

**Researched:** 2026-06-26
**Domain:** VCV Rack (C++17) display/GUI-thread refactor — lock-free audio→GUI hand-off, frame-rate-independent animation, headless determinism testing
**Confidence:** HIGH (extraction mechanics, dt math, thread pattern) / MEDIUM (`getLastFrameDuration()` exact semantics — locked decision, flagged for empirical confirmation)

<user_constraints>
## User Constraints (from 24-CONTEXT.md)

### Locked Decisions

**CLEAN-05 — Display buffer off the audio thread**
- **D-01: Hybrid dirty-flag.** `process()` keeps the cheap trigger detection (phase-wrap via `core.phase`, the ~30fps param-change throttle) but only **sets an atomic "dirty" flag**. The GUI widget's `step()` reads the flag and runs the expensive `DISPLAY_SAMPLES` fill. Proven trigger logic is untouched; only the heavy loop crosses the thread boundary, behind the existing lock-free double-buffer + `displayReadIdx` atomics.
- **D-02: Snapshot at trigger.** When the audio thread sets the flag, it snapshots the exact inputs the preview shape depends on — `morph`, `character`, effective `swingFrac` (the `t.isClocked ? swingFrac : 0.5f` gate), and the live `bleedLfo` (= `core.drift.ouLayers[0].state`) — into a small lock-free struct. The rendered preview shape must stay **bit-identical** to today, including the live drift state captured **at the trigger instant** (NOT re-read live at paint time).

**CLEAN-04 — Frame-rate-independent animations**
- **D-03: Convert ALL time-based animations to clamped wall-clock dt**, including the ANIM-02-locked badge decay. Replace per-frame `flashIntensity *= 0.92f` with `pow(0.92, dt*60)` — **mathematically identical at 60fps** yet frame-rate-independent. Same treatment for breathe, blink, scanline scroll, and the 200ms fade timers. ANIM-02 lock respected via feel-preservation, never re-tuning.
- **D-04: dt source = `APP->window->getLastFrameDuration()`**, **clamped to ~33ms (1/30s)** to cap pathological large dt after a window stall / tab-out. The mandated test asserts animations advance only by the clamped amount.

**CLEAN-03 — Pill fade-out symmetry**
- **D-05: Widget-side cache.** Cache last valid `ratioIdx` + period; replace the `ratioIdx >= 0` early-return with an alpha gate so ratio/BPM pills fade with the SYNC badge. **No atomic/DSP changes** (the "hold the atomics" alternative was rejected). Hz and Swing overlays already correct — leave them.

**Verification gate**
- **D-06:** Headless display-buffer determinism test (byte-identical buffer for the same snapshot inputs regardless of thread) + **mandated** CLEAN-04 pathological-dt test + manual in-Rack UAT for visual feel.

**CLEAN-01 / CLEAN-02 — mechanical**
- **D-07:** Remove dead code `drawZeroCrossing`, `scanlineImage` (CLEAN-01). Resolve unreachable `isStill` (CLEAN-02). No design decision needed.

### Claude's Discretion
- Exact dirty-flag/snapshot struct layout and memory-ordering; precise clamp constant expression; whether the headless test lives in the existing doctest/BlockDriver harness or a new file; test naming/organization; how `isStill` is resolved (remove vs. wire to a reachable condition). All implementation details for researcher/planner.

### Deferred Ideas (OUT OF SCOPE)
- RNG strategy (resolved P22 D-07 — do NOT re-open). Full DSP core extraction + `process()` thinning + `bleedLfo` lift (already done P22 D-02/D-04/D-05).
- IP/font git-history purge, LICENSE/NOTICES → Phase 25.
- VCV manifest validation, `.vcvplugin` packaging, CI validation → Phase 26.
- `swingIndex` GUI→audio non-atomic write — pre-existing tech debt; not in scope unless it surfaces while touching display code.
- Any new DSP behavior (LFO feature-frozen).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CLEAN-01 | Remove dead code `drawZeroCrossing()` + `scanlineImage` | **VERIFIED dead** by grep: `drawZeroCrossing` has only its definition (`AnalogLFO.cpp:529`), no caller; `scanlineImage` has only its declaration (`:373`), no use. Mechanical delete. |
| CLEAN-02 | Resolve unreachable `isStill` dim condition | `isStill` (`:974`) = `rate <= 0.001f` but `RATE_PARAM` min is `0.01f` (`:194`) → never true. Recommend delete (REQUIREMENTS forbids lowering param min). |
| CLEAN-03 | Ratio/BPM pills fade symmetrically with SYNC badge | Widget-side cache pattern below; no atomic change. Cache `ratioIdx` + period in `step()`, alpha-gate the draw. |
| CLEAN-04 | Frame-rate-independent animations via wall-clock dt | dt conversion table + clamp expression + extracted pure helpers for headless testing, below. |
| CLEAN-05 | Display buffer regeneration on GUI thread | Seqlock snapshot + GUI-side fill; extract fill to `src/dsp/DisplayFill.hpp` as a pure function. |

**Requirement-ownership confirmation (TEST-02):** TEST-02 ("full Rack-independent core extraction") was **satisfied in Phase 22** (D-02/D-04). `process()` already delegates the whole per-sample chain to `core.step()` (`AnalogLFO.cpp:315`); the core lives in `src/dsp/*.hpp`; the `bleedLfo` lift is done (`Waveshape::morphedWave` param, `core.drift.ouLayers[0].state` passed at `:184`). **ROADMAP Phase 24 success criteria #1 (extraction/thinning) and #2 (DSP output identical) are already green** via the golden harness (`tests/test_golden.cpp`). Do NOT re-plan extraction. The ROADMAP "Open decision: RNG strategy" is **stale** (resolved P22 D-07). This phase = CLEAN-01..05 + the D-06 verification only.
</phase_requirements>

## Summary

Phase 24 is a **GUI-thread / display-layer refactor on a frozen DSP core**. The DSP is done and proven byte-stable by the golden harness; the five cleanups touch only the Rack shell's `WaveformDisplay` widget and the small bridge between `process()` and that widget. Three of the five (CLEAN-01/02/03) are mechanical or self-contained widget edits. The genuine engineering is in **CLEAN-05** (move a ~256×`morphedWave` loop off the real-time audio thread without tearing the snapshot it consumes) and **CLEAN-04** (convert five frame-paced animation accumulators to wall-clock dt while preserving the ANIM-02-locked badge decay's feel exactly).

The single most leverage-y move is to **extract the pure preview-fill loop into `src/dsp/DisplayFill.hpp`** and the **dt clamp + decay math into a tiny `src/dsp/Anim.hpp`**. This (a) gives the GUI widget and the Rack-free test harness one shared definition, (b) *structurally* enforces "capture-at-trigger, never read live drift at paint" because `bleedLfo` becomes a function parameter the GUI fill cannot bypass, and (c) makes D-06's "byte-identical buffer" and "pathological-dt" claims testable headlessly with the existing doctest/BlockDriver infrastructure.

For the thread hand-off, use a **seqlock** (a single `std::atomic<uint32_t>` sequence counter that doubles as the dirty flag) over a 16-byte POD snapshot. It is wait-free for the audio producer, tear-free for the GUI consumer, costs ~6 lines, and matches the project's existing release/acquire atomics idiom while strictly honoring D-02's "no torn reads."

**Primary recommendation:** Extract `forge::fillDisplayBuffer()` (pure, `bleedLfo` as explicit param) and `forge::clampFrameDt()` / `forge::flashDecay()` into header-only `src/dsp/` helpers; bridge `process()`→widget with a seqlock snapshot; convert every `X / 60.f` increment to `X * dt` and the decay to `*= pow(0.92f, dt*60.f)`; cache `ratioIdx`+period widget-side; delete `drawZeroCrossing`/`scanlineImage`/`isStill`. Verify with two headless doctest cases + a manual in-Rack UAT.

## Architectural Responsibility Map

Tiers here are **audio thread** (`Module::process()`, real-time), **GUI thread** (`Widget::step()`/`drawLayer()`), and **`src/dsp/` pure layer** (Rack-free, shared by shell + tests).

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Trigger detection (phase-wrap, 30fps throttle) | Audio thread | — | D-01: proven logic stays put; only a flag is set |
| Snapshot publish (morph/char/swing/bleedLfo) | Audio thread | `src/dsp` (struct def) | D-02: capture-at-trigger; audio owns the publish |
| Display-buffer fill (256× `morphedWave`) | GUI thread (`step()`) | `src/dsp` (pure fn) | CLEAN-05: heavy loop leaves the real-time path |
| Animation advance (breathe/blink/scanline/fade/flash) | GUI thread (`step()`) | `src/dsp` (dt+decay helpers) | CLEAN-04: `getLastFrameDuration()` is GUI-thread-only |
| Pill fade-out cache (ratioIdx/period) | GUI thread (`step()`+draw) | — | CLEAN-03: pure draw-layer fix, no atomics touched |
| Dead-code removal / `isStill` | GUI thread (widget) | — | CLEAN-01/02: mechanical widget hygiene |
| Pure fill + clamp + decay math | `src/dsp` | — | D-06: must be callable from the Rack-free harness |

## Standard Stack

No external packages. This is an internal C++17 refactor using the standard library and the Rack SDK already in the tree. **No `## Package Legitimacy Audit` needed** (zero installs).

### Core (already in use)
| Facility | Source | Purpose | Why standard |
|----------|--------|---------|--------------|
| `std::atomic<uint32_t>` | `<atomic>` (already included `AnalogLFO.cpp:5`) | Seqlock sequence counter / dirty flag | Lock-free SPSC; no new dependency |
| `std::atomic_thread_fence` | `<atomic>` | Seqlock acquire/release fences | Textbook seqlock correctness |
| `std::pow` | `<cmath>` (already included `:4`) | `pow(0.92f, dt*60.f)` continuous decay | Exact continuous-time equivalent of geometric per-frame decay |
| `std::isfinite` | `<cmath>` | Guard `getLastFrameDuration()` NAN first-frame | Required (see Pitfall 1) |
| `rack::window::Window::getLastFrameDuration()` | Rack SDK `window/Window.hpp:102` | Wall-clock dt for animations (D-04 locked) | The Rack-idiomatic per-frame dt |
| `forge::Waveshape::morphedWave` | `src/dsp/Waveshape.hpp:156` | The preview shape (already `const`, `bleedLfo` param) | Single source of truth, shared with audio path |

### Supporting (new header-only helpers — recommended)
| File | Purpose | When to use |
|------|---------|-------------|
| `src/dsp/DisplayFill.hpp` | `forge::fillDisplayBuffer(out, wave, morph, char, swingFrac, bleedLfo)` — pure preview fill + swing remap | Called by widget `step()` AND the headless determinism test |
| `src/dsp/Anim.hpp` | `forge::clampFrameDt(raw)`, `forge::flashDecay(intensity, dt)` (+ optional rate constants) | Called by widget `step()` AND the pathological-dt test |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Seqlock snapshot | 2-slot snapshot + publish-index (mirrors existing `displayBuffers[2]` exactly) | Lower rigor: same theoretical 2-slot tear the existing buffer already tolerates; acceptable but violates D-02 "no torn reads" letter. Seqlock is strictly tear-free for ~same cost. |
| Seqlock snapshot | `std::mutex` / `SpinLock` | Forbidden on the audio thread — can block the real-time callback. Rejected. |
| Per-field `std::atomic<float>` ×4 | — | Risks torn reads *across* fields (new morph + old bleedLfo) → a buffer the audio thread never would have produced. Rejected per D-02. |
| New `src/dsp/DisplayFill.hpp` | Inline the fill in the widget only | Then the loop is not callable from the Rack-free harness → D-06's byte-identical test cannot run headlessly. Rejected. |

**No installation step.** All facilities are stdlib or already-vendored Rack SDK / `src/dsp/`.

## Architecture Patterns

### System Architecture Diagram

```
                         AUDIO THREAD (process(), real-time)
  Rack params/inputs ─► core.step(in) ─► outputVoltage ─► outputs[OUTPUT]
                              │
                              ├─► publish display atomics (relaxed)  ── unchanged ──┐
                              │   (clockState, ratioIdx, period, phase, swing…)      │
                              │                                                       ▼
                      trigger? (phaseWrap || (paramChange && 30fps throttle))   ┌──────────┐
                              │  YES                                            │  display │
                              ▼                                                │ atomics  │
                   ┌─────────────────────────────┐                            └────┬─────┘
                   │ SEQLOCK PUBLISH (NEW)        │                                 │ acquire
                   │  seq++ (odd)                 │                                 │
                   │  snapshot = {morph,char,     │                                 │
                   │    effSwing, bleedLfo=        │                                 │
                   │    core.drift.ouLayers[0]    │   capture-at-trigger            │
                   │      .state}                 │                                 │
                   │  seq++ (even, release)       │                                 │
                   └──────────────┬──────────────┘                                 │
   . . . . . . . . . . . . . . . .│. . . . . . . . . . . . . . . . . . . . . . . . │. . .
                                  │ seqlock (acquire / retry)                       │
                        GUI THREAD (WaveformDisplay::step(), once/frame)            │
                                  ▼                                                 │
                   read seq; copy snapshot; re-read seq; retry if changed/odd       │
                                  │                                                 │
                  seq advanced? ──┤ YES                                             │
                                  ▼                                                 │
              forge::fillDisplayBuffer(displayBuffers[1-readIdx], core.wave,        │
                                       snap.morph, snap.character,                  │
                                       snap.swingFrac, snap.bleedLfo)               │
                                  │                                                 │
                          displayReadIdx.store(writeIdx, release)  ── unchanged ────┘
                                  │
              dt = clampFrameDt(APP->window->getLastFrameDuration())                │
              breathe/blink/scanline/fade += rate*dt ; flash *= pow(.92,dt*60)      │
                                  │                                                 ▼
                        drawLayer(): read displayReadIdx (acquire) → draw trace/dot/pills
                                    (CLEAN-03 cache gates ratio/BPM pill fade)
```

### Component Responsibilities
| File / symbol | Responsibility after Phase 24 |
|---------------|-------------------------------|
| `AnalogLFO::process()` (`:332-351`) | Trigger detection unchanged; **replace** `updateDisplayBuffer(...)` call with seqlock snapshot publish |
| `AnalogLFO` (new members) | `struct DisplaySnapshot`, `DisplaySnapshot displaySnapshot`, `std::atomic<uint32_t> displaySnapshotSeq{0}` |
| `AnalogLFO::updateDisplayBuffer` (`:167`) | Becomes thin `fillFromSnapshot()` wrapper that calls `forge::fillDisplayBuffer` into `displayBuffers[writeIdx]` and publishes `displayReadIdx` |
| `src/dsp/DisplayFill.hpp` (NEW) | `forge::fillDisplayBuffer` — pure loop + swing remap; `bleedLfo` is a parameter |
| `src/dsp/Anim.hpp` (NEW) | `forge::clampFrameDt`, `forge::flashDecay` |
| `WaveformDisplay::step()` (`:386`) | Compute `dt` once; advance all anims via dt; consume seqlock + run fill; cache ratioIdx/period |
| `WaveformDisplay::drawTextOverlays`/`drawBpmStack` | Use cached ratioIdx/period; alpha-gate instead of `ratioIdx >= 0` |
| `WaveformDisplay::drawLayer` (`:974`) | Drop `isStill`; `dimFactor = isBypassed() ? 0.25f : 1.f` |

### Pattern 1: Seqlock snapshot (single-producer audio, single-consumer GUI)
**What:** A `std::atomic<uint32_t>` sequence counter that is *even* when the payload is stable and *odd* while the producer writes. The counter also serves as the dirty flag (consumer compares against its last-consumed value).
**When to use:** Publishing a multi-field POD from a real-time producer to a non-real-time consumer where the producer must never block. [CITED: classic seqlock; consistent with the codebase's existing release/acquire `displayReadIdx`]

```cpp
// ---- on the AnalogLFO module ----
struct DisplaySnapshot {        // 16 bytes, plain POD
    float morph     = 0.f;
    float character = 0.f;
    float swingFrac = 0.5f;     // EFFECTIVE (gated) value — never recomputed GUI-side
    float bleedLfo  = 0.f;      // core.drift.ouLayers[0].state captured AT TRIGGER
};
DisplaySnapshot displaySnapshot;                 // non-atomic payload
std::atomic<uint32_t> displaySnapshotSeq{0};     // even=stable, odd=writing; also the dirty flag

// ---- audio producer, inside process() where the fill used to be called (:343-350) ----
if (phaseWrapped || ((morphChanged || characterChanged || swingChanged) && paramReady)) {
    uint32_t s = displaySnapshotSeq.load(std::memory_order_relaxed);
    displaySnapshotSeq.store(s + 1, std::memory_order_relaxed);          // begin (odd)
    std::atomic_thread_fence(std::memory_order_release);
    displaySnapshot.morph     = morph;
    displaySnapshot.character = character;
    displaySnapshot.swingFrac = t.isClocked ? t.swingFrac : 0.5f;        // same gate as today (:344)
    displaySnapshot.bleedLfo  = core.drift.ouLayers[0].state;            // capture-at-trigger (was read live in fill)
    displaySnapshotSeq.store(s + 2, std::memory_order_release);          // commit (even)
    prevDisplayMorph = morph;  prevDisplayCharacter = character;
    prevSwingIndex = swingIndex;  displayUpdateTimer = 0.f;              // bookkeeping unchanged
}

// ---- GUI consumer, inside WaveformDisplay::step() ----
uint32_t seq;
AnalogLFO::DisplaySnapshot snap;
do {
    seq = module->displaySnapshotSeq.load(std::memory_order_acquire);
    if (seq & 1u) continue;                                    // writer mid-update → re-read
    snap = module->displaySnapshot;                            // copy 16 bytes
    std::atomic_thread_fence(std::memory_order_acquire);
} while (seq != module->displaySnapshotSeq.load(std::memory_order_relaxed));
if (seq != lastConsumedSeq) {                                  // dirty?
    module->fillFromSnapshot(snap);                            // heavy fill, GUI thread
    lastConsumedSeq = seq;
}
```
**Why correct:** writer is wait-free (three stores + payload, no loop); reader either copies a fully-published payload or detects an in-progress/changed write and retries. Producer cadence is bounded (≤ ~20 phase-wraps/s at the 20 Hz rate ceiling, plus throttled ≤30/s param changes), so the reader effectively never spins more than once. Memory ordering: the closing `release` store happens-before the reader's `acquire` load, so the reader sees all four field writes. [VERIFIED: reasoning against the seqlock contract + the SPSC roles in this codebase]

### Pattern 2: Pure preview fill in `src/dsp/DisplayFill.hpp`
**What:** Lift the `updateDisplayBuffer` loop body (`:169-185`) verbatim into a Rack-free `inline` free function; `bleedLfo` is an explicit parameter (mirrors the D-05 `morphedWave` lift already done).
**When to use:** Whenever a render computation must be shared by the GUI widget and a headless test, and must be provably free of live/global reads.

```cpp
#pragma once
#include <array>
#include "dsp/Waveshape.hpp"
namespace forge {
constexpr int DISPLAY_SAMPLES = 256;
// Pure: identical (snapshot, wave-spreads) -> identical buffer, on ANY thread.
// No live/global state — bleedLfo is a parameter, so the GUI fill structurally
// CANNOT read core.drift.ouLayers[0].state at paint time (D-02).
inline void fillDisplayBuffer(std::array<float, DISPLAY_SAMPLES>& out,
                              const Waveshape& wave,
                              float morph, float character,
                              float swingFrac, float bleedLfo) {
    for (int i = 0; i < DISPLAY_SAMPLES; ++i) {
        float t = (float)i / (float)DISPLAY_SAMPLES;
        float p;
        if (swingFrac <= 0.5001f)  p = t;                                   // fast path
        else if (t < swingFrac)    p = t * 0.5f / swingFrac;                // even half
        else                       p = 0.5f + (t - swingFrac) * 0.5f / (1.f - swingFrac); // odd half
        out[i] = wave.morphedWave(p, morph, character, bleedLfo);
    }
}
} // namespace forge
```
The module wrapper keeps the double-buffer publish identical to today:
```cpp
void fillFromSnapshot(const DisplaySnapshot& s) {
    int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
    forge::fillDisplayBuffer(displayBuffers[writeIdx], core.wave,
                             s.morph, s.character, s.swingFrac, s.bleedLfo);
    displayReadIdx.store(writeIdx, std::memory_order_release);   // unchanged (:186)
}
```

### Pattern 3: Wall-clock dt conversion (CLEAN-04)
Every animation is a **linear accumulator/ramp** except the flash decay, which is **geometric**. Linear terms: replace `X / 60.f` (an implicit "per 1/60 s" increment) with `X * dt`. Geometric term: replace `*= 0.92f` with `*= pow(0.92f, dt*60.f)`.

| Site (line) | Current (frame-paced) | Continuous-time (dt) | Check at dt = 1/60 |
|-------------|------------------------|----------------------|--------------------|
| breathe (`:388`) | `breathePhase += 2π·0.2 / 60` | `breathePhase += 2π·0.2f * dt` | `2π·0.2/60` ✓ |
| blink (`:392`) | `blinkPhase += 2π·2 / 60` | `blinkPhase += 2π·2f * dt` | `2π·2/60` ✓ |
| scanline (`:396`) | `scanlineScrollPhase += 1 / 60` | `scanlineScrollPhase += dt` | `1/60` ✓ |
| fade step (`:402`) | `fadeSpeed = 1 / (0.2·60)` | `fadeSpeed = dt / 0.2f` | `1/(0.2·60)` ✓ |
| flash decay (`:429`) | `flashIntensity *= 0.92f` | `flashIntensity *= pow(0.92f, dt*60.f)` | `pow(0.92,1)=0.92` ✓ |

Wrap/clamp operations (`if breathePhase > 2π` `:389`; `if blinkPhase > 100π` `:393`; `fmodf(...,4.f)` `:397`; the `rack::math::clamp(..., -fadeSpeed, fadeSpeed)` ramp) are amplitude-preserving and unchanged. dt is computed once at the top of `step()`:

```cpp
// src/dsp/Anim.hpp
#pragma once
#include <cmath>
namespace forge {
constexpr float kMaxFrameDt = 1.f / 30.f;           // D-04 clamp: ~2 nominal frames
inline float clampFrameDt(float raw) {
    if (!(raw == raw) || raw < 0.f) return 0.f;     // NAN (first frame) / negative -> no advance
    return raw < kMaxFrameDt ? raw : kMaxFrameDt;
}
inline float flashDecay(float intensity, float dt) {
    return intensity * std::pow(0.92f, dt * 60.f);  // ANIM-02 0.92 preserved by equivalence
}
} // namespace forge

// WaveformDisplay::step() top:
float dt = forge::clampFrameDt((float)APP->window->getLastFrameDuration());
```
> Note the explicit NAN guard uses `raw == raw` (false for NAN) rather than `rack::math::clamp`, because `rack::math::clamp(NAN, 0, 1/30)` returns **1/30**, not 0 — see Pitfall 1.

### Pattern 4: Widget-side ratio/BPM cache (CLEAN-03)
```cpp
// WaveformDisplay members:
int   cachedRatioIdx = -1;
float cachedPeriod   = 0.f;

// in step(): refresh cache only while genuinely clocked
int   ri  = module->displayRatioIndex.load(std::memory_order_relaxed);
float per = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
if (ri >= 0 && per > 0.f) { cachedRatioIdx = ri; cachedPeriod = per; }

// drawTextOverlays ratio pill (replaces ':891' `ratioFadeAlpha > 0.001f && ratioIdx >= 0`):
if (ratioFadeAlpha > 0.001f && cachedRatioIdx >= 0) {
    drawPillText(..., AnalogLFO::RATIO_LABELS[cachedRatioIdx], ..., ratioFadeAlpha);
}
// drawBpmStack: pass cachedRatioIdx/cachedPeriod IN (stop reading the atomics + the `ratioIdx<0` return at :742)
```
The audio thread keeps publishing `-1` on disconnect (no atomic change — the rejected "hold the atomics" alternative). The pill now fades over the same 200ms ramp as `syncFadeAlpha` because the cached content stays drawable while `ratioFadeAlpha`/`bpmFadeAlpha > 0.001f`.

### Anti-Patterns to Avoid
- **Reading `core.drift.ouLayers[0].state` from the GUI fill.** This is the live-drift-at-paint bug D-02 forbids. The `bleedLfo` parameter on `fillDisplayBuffer` makes it structurally impossible — keep it a parameter.
- **Recomputing the swing gate GUI-side.** The GUI has no `t.isClocked`; the *effective* `swingFrac` must be captured in the snapshot (already in the struct).
- **`std::mutex`/spinlock on the audio thread.** Can block the real-time callback. Use the seqlock.
- **Asserting bit-identity of the new decay to the old frame-paced `*=0.92`.** `pow(0.92f,1.0f)` may differ from the literal `0.92f` by ≤1 ULP (Pitfall 5). D-03 requires feel-equivalence, not bit-equivalence.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Audio→GUI buffer transfer | A new ring buffer / triple buffer | The existing `displayBuffers[2]` + `displayReadIdx` (`:101-102`) — only the *writer* moves to the GUI thread | Already lock-free and shipped through 4 milestones; CLEAN-05 reuses it verbatim |
| Consistent multi-field snapshot | Per-field atomics + ad-hoc ordering | Seqlock over one `std::atomic<uint32_t>` | Tear-free with a wait-free producer; textbook SPSC pattern |
| Continuous decay from per-frame decay | A lookup table / hand-rolled exp | `std::pow(0.92f, dt*60.f)` | Exact, one call/frame, negligible cost |
| First-frame dt sentinel | Magic number | `getLastFrameDuration()` + `clampFrameDt` NAN guard | The SDK *documents* NAN before the first finished frame |
| Preview shape math | A second copy of the waveshape loop | `forge::Waveshape::morphedWave` via `fillDisplayBuffer` | Single source of truth shared with the audio path; ODR-safe |

**Key insight:** every "new" mechanism this phase needs already exists in the codebase or stdlib — the work is *relocation* (loop to GUI thread, math to `src/dsp/`) and *correct fencing*, not invention.

## Runtime State Inventory

This is a thread/display refactor, **not** a rename/migration. No persisted or external state changes.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | **None** — `dataToJson`/`dataFromJson` (`:226-261`) persist only `spreadSeed0/1` + `swingIndex`; no display/snapshot/animation state is serialized | None |
| Live service config | **None** — no external services; offline VCV plugin | None |
| OS-registered state | **None** — no OS registrations | None |
| Secrets/env vars | **None** | None |
| Build artifacts | New headers `src/dsp/DisplayFill.hpp`, `src/dsp/Anim.hpp` are picked up by `TEST_HEADERS := $(wildcard src/dsp/*.hpp)` (Makefile `:34`) and `-Isrc` (`:6`) automatically — no Makefile edit needed; plugin build globs `src/*.cpp` only, so header-only additions need no source-list change | Verify `make` + `make test` both still build (headers are include-only) |

**In-process runtime state introduced (not persisted):** the seqlock counter + snapshot POD live on the `AnalogLFO` module instance, reset on construction; the widget cache (`cachedRatioIdx`, `lastConsumedSeq`, animation phases) lives on the widget. None survive a patch save/load — correct, matching the existing display atomics.

## Common Pitfalls

### Pitfall 1: `getLastFrameDuration()` returns NAN on the first frame(s) — and `rack::math::clamp(NAN)` returns the MAX
**What goes wrong:** `Window::getLastFrameDuration()` is documented to "return NAN if no frames have ended rendering" (`window/Window.hpp:100`). If you clamp it with `rack::math::clamp(dt, 0.f, 1.f/30.f)`, the result on a NAN input is **`1/30`, not `0`** — so every animation jumps a full clamped step on the first frame.
**Why it happens:** `rack::math::clamp(x,a,b) = std::fmax(std::fmin(x,b),a)` (verified, `math.hpp:106-108`). `std::fmin(NAN, 1/30)` returns the non-NAN operand `1/30`; `std::fmax(1/30, 0)` = `1/30`.
**How to avoid:** explicit finite guard *before* clamping: `if (!(raw==raw) || raw < 0.f) dt = 0.f; else dt = std::min(raw, 1.f/30.f);` (this is `forge::clampFrameDt`). [VERIFIED: SDK docstring + `rack::math::clamp` source]
**Warning signs:** a one-frame "pop" in breathe/blink the instant a module is added.

### Pitfall 2: `getLastFrameDuration()` semantics — "render time" vs "inter-frame interval"
**What goes wrong:** The SDK docstring says "total time in seconds spent **rendering** the last frame." If it literally measured only active render time (≪ the vsync interval), it would be a *wrong* dt and animations would run slow / render-load-dependent rather than wall-clock-stable.
**Why it (likely) does NOT happen:** In Rack v2's frame loop, `lastFrameDuration` is in practice the wall-clock delta between consecutive frame starts (≈ `1/refreshRate` under vsync), which is the correct animation dt — this is the established community usage of the call for frame-rate-independent animation, and the basis for CODE-REVIEW #11's recommendation. Confidence MEDIUM (docstring is loose; not bit-confirmed from source — SDK ships headers only).
**How to avoid / verify:** The decision is **locked** (D-04) — do not re-open. During the manual in-Rack UAT, log `getLastFrameDuration()` once and confirm it reads ≈ `1/refreshRate` (e.g. ~0.0167 at 60 Hz, ~0.0069 at 144 Hz), **not** a sub-millisecond render time. If it reads render-only time, raise it to the operator before closing (fallback: derive dt from `getFrameTime()` deltas across `step()`). [CITED: window/Window.hpp:99-102]
**Warning signs:** breathe cycle visibly longer than 5 s, or its speed changing when other modules are added (render-load coupling).

### Pitfall 3: Capturing `bleedLfo` at paint instead of at trigger
**What goes wrong:** If the GUI fill reads `module->core.drift.ouLayers[0].state` live, the preview is sampled at a *different* drift phase than today's audio-thread fill captured — the shape drifts vs the shipped behavior, breaking D-02 byte-identity.
**How to avoid:** snapshot `core.drift.ouLayers[0].state` in `process()` at the trigger instant (in the struct); make `bleedLfo` a parameter of `fillDisplayBuffer` so the GUI *cannot* read it live. The extraction enforces this at compile time.
**Warning signs:** the preview "breathes" differently from before even at fixed morph/character.

### Pitfall 4: `core.wave` spread members read by the GUI thread
**What goes wrong:** `fillDisplayBuffer` reads `core.wave` (the spread coefficients). These are mutated by `setSpreadSeed()` via `dataFromJson()` / construction on the **main thread**. A GUI read concurrent with a patch-load write is a theoretical data race.
**Why it is acceptable:** spreads are written only at construct and patch-load (engine effectively paused during load); the values are stable during normal operation; the audio thread already reads `core.wave` every sample today. This is pre-existing and out of scope.
**How to avoid surprises:** document the assumption; do not add new mid-run writes to `core.wave`. [ASSUMED: patch-load/engine-pause interaction — pre-existing behavior, not changed by this phase]

### Pitfall 5: `pow(0.92f, 1.0f) ≠ 0.92f` bit-for-bit
**What goes wrong:** asserting the converted decay is *bit-identical* to the old `*= 0.92f` at 60fps will fail by ≤1 ULP.
**How to avoid:** D-03 requires "mathematically identical at 60fps" / feel-preservation, not bit-identity. Test with `doctest::Approx(0.92f)` for the 60fps equivalence check, never `==`.

### Pitfall 6: ODR / single-definition for the new headers
**What goes wrong:** non-`inline` free functions in a header included by multiple TUs (the shell + several test `.cpp`s) cause duplicate-symbol link errors (the project's "Pitfall 4" from prior phases).
**How to avoid:** mark every free function `inline`; constants `constexpr`. Single-source `DISPLAY_SAMPLES` in `forge::` and have the shell reference `forge::DISPLAY_SAMPLES` (or keep its `static constexpr` aliasing it) so the `256` literal lives in one place.

### Pitfall 7: Cross-build float divergence is NOT "byte-identical" — scope the D-06 claim correctly
**What goes wrong:** The test binary builds with `-ffp-contract=off` (Makefile `:37`); the plugin builds with the SDK's flags (may allow FMA contraction). So a buffer produced by the *plugin* and one produced by the *test binary* can differ in the low mantissa even for identical inputs — exactly the caveat already documented in `tests/test_golden.cpp:11-22`.
**How to avoid:** D-06's "byte-identical buffer regardless of which thread fills it" is correctly interpreted as **same-binary thread-independence + fill purity**: within the plugin binary, moving the *same* compiled `fillDisplayBuffer` from the audio thread to the GUI thread yields literally identical machine output (bit-identical by construction). The headless test asserts (a) **purity/determinism** (same inputs → bit-identical output across repeated calls in the test binary) and (b) optionally pins a committed reference buffer with the same canonical-OS-bit-exact / 1e-5-tolerance split the golden test uses. Do **not** write a test that bit-compares the test binary's buffer to the plugin's. [VERIFIED: against `tests/test_golden.cpp` Pitfall 6 precedent + Makefile flags]

## Code Examples

### Headless display determinism test (D-06)
```cpp
// tests/test_display.cpp  (new TU; does NOT define the doctest impl macro — main.cpp owns it)
#include "doctest.h"
#include "dsp/DisplayFill.hpp"
#include "dsp/LfoCore.hpp"
#include <array>

TEST_CASE("display: fill is a pure, thread-independent function of the snapshot") {
    forge::LfoCore core;
    core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);     // canonical golden spread seeds
    std::array<float, forge::DISPLAY_SAMPLES> a{}, b{};
    // Same snapshot, two calls — emulates "audio-thread fill" vs "GUI-thread fill".
    forge::fillDisplayBuffer(a, core.wave, 0.4f, 0.6f, 0.58f, 0.123f);
    forge::fillDisplayBuffer(b, core.wave, 0.4f, 0.6f, 0.58f, 0.123f);
    for (int i = 0; i < forge::DISPLAY_SAMPLES; ++i)
        CHECK(a[i] == b[i]);                              // bit-exact within this binary
}

TEST_CASE("display: swing remap matches the no-swing fast path at Straight") {
    forge::Waveshape w;                                    // zero spreads
    std::array<float, forge::DISPLAY_SAMPLES> straight{}, gated{};
    forge::fillDisplayBuffer(straight, w, 0.5f, 0.3f, 0.50f, 0.f);   // <=0.5001 fast path
    forge::fillDisplayBuffer(gated,    w, 0.5f, 0.3f, 0.5001f, 0.f); // boundary
    for (int i = 0; i < forge::DISPLAY_SAMPLES; ++i) CHECK(straight[i] == gated[i]);
}
```

### Mandated pathological-dt test (CLEAN-04 / D-06)
```cpp
// tests/test_anim.cpp
#include "doctest.h"
#include "dsp/Anim.hpp"
#include <cmath>

TEST_CASE("anim: pathological dt clamps to one cap step") {
    CHECK(forge::clampFrameDt(10.0f)        == doctest::Approx(1.f/30.f)); // huge stall -> cap
    CHECK(forge::clampFrameDt(std::nanf("")) == 0.f);                      // first-frame NAN -> 0
    CHECK(forge::clampFrameDt(-1.f)          == 0.f);                      // negative -> 0
    // a huge dt advances the decay by NO MORE than the clamped step:
    CHECK(forge::flashDecay(1.f, forge::clampFrameDt(10.f))
          == forge::flashDecay(1.f, 1.f/30.f));
}
TEST_CASE("anim: decay is feel-identical at 60fps") {
    CHECK(forge::flashDecay(1.f, 1.f/60.f) == doctest::Approx(0.92f));     // matches old *=0.92
}
```

## State of the Art

| Old Approach (today) | New Approach (Phase 24) | Why |
|----------------------|--------------------------|-----|
| 256× `morphedWave` fill inside `process()` (`:345`, audio thread) | Fill in `WaveformDisplay::step()` (GUI thread) behind a seqlock snapshot | Removes ~15–60µs / ~1500 transcendentals from the real-time callback (CODE-REVIEW #12) |
| Animations advanced by fixed `1/60` ticks (`:388-429`) | Clamped wall-clock dt (`getLastFrameDuration`) | Frame-rate-independent at 144 Hz etc. (CODE-REVIEW #11) |
| Ratio/BPM pills early-return on `ratioIdx < 0` (`:742`,`:891`) | Widget-side cache + alpha gate | Symmetric 200ms fade with the SYNC badge (CODE-REVIEW #10) |
| `drawZeroCrossing`, `scanlineImage`, `isStill` present | Removed / resolved | Dead/unreachable code (CODE-REVIEW #8/#9) |

**Deprecated/outdated:** none external — this is internal hygiene on a frozen feature set.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `getLastFrameDuration()` returns the wall-clock inter-frame interval (≈1/refreshRate), not active render time | Pitfall 2 | Animations would be slow / render-load-coupled. Mitigated: decision is locked (D-04) AND the manual UAT includes a one-line probe to confirm. |
| A2 | Engine is effectively paused during `dataFromJson` patch-load, so the GUI reading `core.wave` spreads is not a live race | Pitfall 4 | A real (but pre-existing, out-of-scope) data race on spread coefficients during load. No new exposure created by this phase. |

## Open Questions

1. **Where do the shared helpers live?**
   - Recommendation: new `src/dsp/DisplayFill.hpp` and `src/dsp/Anim.hpp` (header-only, auto-globbed by the test Makefile). Alternative: fold `Anim.hpp` into an existing header to minimize file count — planner's discretion (D-discretion).
2. **`isStill` resolution: delete vs. wire to effective frequency?**
   - Recommendation: **delete** (`dimFactor = isBypassed() ? 0.25f : 1.f`). REQUIREMENTS forbids lowering the Rate param min; comparing effective clocked frequency widens blast radius for a behavior that has never been reachable. Minimal change. (D-07 discretion.)
3. **Do we commit a reference display buffer (golden) or rely on the purity test?**
   - Recommendation: the purity + structural-no-live-read guarantee is sufficient for D-06's automatable portion; a committed reference buffer is optional belt-and-suspenders. If added, reuse the canonical-OS-bit-exact / 1e-5-tolerance split from `test_golden.cpp` (Pitfall 7).
4. **Seqlock vs. publish-index double-buffer for the snapshot?**
   - Recommendation: seqlock (honors D-02 "no torn reads" strictly). The double-buffer-index variant is acceptable and matches existing code but carries the same theoretical 2-slot tear as `displayBuffers`.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| C++17 toolchain (`$(CXX)`) | `make` + `make test` | ✓ | per system default | — |
| Rack SDK `../Rack-SDK` | plugin build (`make`) | ✓ | v2 (headers present, `window/Window.hpp` confirmed) | — |
| doctest harness | `make test` | ✓ | vendored `tests/doctest.h` | — |
| `make test` currently green | regression baseline | ✓ | 43/43 per STATE.md (Phase 23) | — |

No external installs. New header-only files require no Makefile change (`-Isrc` + `$(wildcard src/dsp/*.hpp)` already cover them).

## Validation Architecture

`workflow.nyquist_validation` is **absent** from `.planning/config.json` → treat as enabled.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | doctest (single-header `tests/doctest.h`) + `forge::BlockDriver` |
| Config file | none — `Makefile` `test` target (`:39-45`), Rack-free, `-ffp-contract=off` |
| Quick run command | `make test` (builds `build-test/test`, runs all TUs) |
| Full suite command | `make test` (same; ~sub-second) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CLEAN-05 | Fill is a pure, thread-independent fn of the snapshot | unit | `make test` (`display: fill is a pure...`) | ❌ Wave 0 (`tests/test_display.cpp`) |
| CLEAN-05 | Swing remap correct at the 0.5001 boundary | unit | `make test` | ❌ Wave 0 |
| CLEAN-04 | Pathological dt clamps to one cap step; NAN/neg → 0 | unit | `make test` (`anim: pathological dt...`) | ❌ Wave 0 (`tests/test_anim.cpp`) |
| CLEAN-04 | Decay feel-identical at 60fps | unit | `make test` (`anim: decay is feel-identical...`) | ❌ Wave 0 |
| CLEAN-03 | Ratio/BPM pill fade symmetry | manual UAT | in-Rack: disconnect clock, observe symmetric fade | n/a (visual, human-gated per D-06) |
| CLEAN-01/02 | Dead code gone, `isStill` resolved | compile + manual | `make` builds clean; in-Rack visual unchanged | n/a |
| All | Overall visual feel (animations, fades, pills) | manual UAT | in-Rack audition before close | n/a (D-06) |

### Sampling Rate
- **Per task commit:** `make test`
- **Per wave merge:** `make test` + `make` (plugin still builds against `../Rack-SDK`)
- **Phase gate:** `make test` green + manual in-Rack UAT logged to STATE.md before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `tests/test_display.cpp` — covers CLEAN-05 (fill purity + swing remap). Depends on `src/dsp/DisplayFill.hpp`.
- [ ] `tests/test_anim.cpp` — covers CLEAN-04 (clamp + decay). Depends on `src/dsp/Anim.hpp`.
- [ ] Framework install: none (doctest vendored; new TUs auto-globbed by `TEST_SOURCES := $(wildcard tests/*.cpp)`).

## Security Domain

`security_enforcement` is absent from config; this is an offline VCV plugin GUI refactor with no network, auth, persistence, or untrusted input introduced. ASVS categories (V2/V3/V4/V5/V6) are **not applicable** — no new inputs, no crypto, no access control. The one safety-critical concern is **concurrency correctness / undefined behavior**, addressed by the seqlock memory-ordering analysis (Pattern 1) and Pitfalls 1–7. Patch-JSON robustness was already hardened in Phase 23 (BUG-04, `forge::parseSeedHex`) and is untouched here.

| Concern | Category | Mitigation |
|---------|----------|------------|
| Data race on audio→GUI snapshot | Concurrency / UB | Seqlock with release/acquire fences; wait-free producer |
| Torn read of multi-field snapshot | Concurrency / UB | Seqlock retry loop (D-02 "no torn reads") |
| Real-time thread blocking | Audio safety | No locks on audio thread; heavy loop moved to GUI thread |
| First-frame NAN dt → undefined animation jump | Robustness | `clampFrameDt` finite guard |

## Sources

### Primary (HIGH confidence)
- `src/AnalogLFO.cpp` (full read) — every cited line number verified against the working tree (updateDisplayBuffer `:167-187`, fill call `:345`, snapshot/atomics `:316-330`, animations `:386-431`, fade loop `:399-419`, ratio pill `:891`, `drawBpmStack` `:739-852`, `drawZeroCrossing` `:529`, `scanlineImage` `:373`, `isStill` `:974`).
- `src/dsp/Waveshape.hpp:156` — `morphedWave(phase, morph, character, bleedLfo)` already takes the lifted `bleedLfo` param (D-05 done).
- `src/dsp/DriftEngine.hpp:118,133,164` — `bleedLfo` = `ouLayers[0].state` (retained, not zeroed).
- `src/dsp/LfoCore.hpp` — `core.wave`, `core.drift`, `core.phase`, `Telemetry` surface confirmed.
- `tests/BlockDriver.hpp`, `tests/test_golden.cpp`, `Makefile:31-45` — harness shape, `-ffp-contract=off`, cross-build tolerance precedent.
- `../Rack-SDK/include/window/Window.hpp:95-104` — `getLastFrameDuration()` docstring (render time; NAN before first finished frame); `getFrameTime()` fallback.
- `../Rack-SDK/include/math.hpp:106-108` — `clamp(x,a,b)=fmax(fmin(x,b),a)` (NAN→max behavior verified).
- `.planning/phases/24-dsp-extraction-display-refactors/24-CONTEXT.md` — locked decisions D-01..D-07.
- `CODE-REVIEW-FINDINGS.md` §8–§12 — the five findings being fixed.

### Secondary (MEDIUM confidence)
- VCV Rack API docs (`vcvrack.com/docs-v2` Window struct) — confirms `getLastFrameDuration()` exists; did not surface exact render-vs-interval semantics (Pitfall 2, A1).

### Tertiary (LOW confidence)
- Community usage convention that `getLastFrameDuration()` is the standard per-frame dt for frame-rate-independent animation — basis for CODE-REVIEW #11's recommendation; flagged for empirical UAT confirmation.

## Metadata

**Confidence breakdown:**
- Seqlock thread hand-off: HIGH — standard SPSC seqlock; memory ordering reasoned against the exact producer/consumer roles in this codebase.
- dt conversion math: HIGH — each transform algebraically checked at dt=1/60; pure-helper extraction verified testable against the existing harness.
- `getLastFrameDuration()` exact semantics: MEDIUM — SDK ships headers only; docstring says "render time" but established usage/CODE-REVIEW intent is inter-frame interval. Decision is locked; flagged for UAT probe.
- Fill/dead-code mechanics: HIGH — every line verified by direct read + grep; dead code confirmed unreferenced.
- Headless byte-identical scoping: HIGH — bounded by the documented cross-build float caveat already in `test_golden.cpp`.

**Research date:** 2026-06-26
**Valid until:** 2026-07-26 (stable internal codebase; only risk is a Rack SDK update changing `getLastFrameDuration()` — pinned to local `../Rack-SDK`)
