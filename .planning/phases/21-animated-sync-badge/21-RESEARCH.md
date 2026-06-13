# Phase 21: Animated SYNC Badge - Research

**Researched:** 2026-06-13
**Domain:** VCV Rack C++ plugin — lock-free DSP→widget signaling + NanoVG per-frame envelope animation
**Confidence:** HIGH (all claims verified directly against src/AnalogLFO.cpp this session)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **ANIM-01** — SYNC badge flashes on each clock edge with a bright pulse.
- **ANIM-02** — Flash uses exponential decay (~0.92× per frame) back to steady state
  (≈200ms). The decay mechanism is fixed by requirement — not a decision to revisit.
- Badge position/size are final from Phase 20.1 (18HP panel) — flash must respect them.
- **D-01** — Flash reads as "brighter than steady-state" via **color shift toward
  white-hot PLUS a glow bloom**, applied together at the peak. Steady state already
  renders at full ember alpha (1.0), so brightness CANNOT come from alpha alone. Both
  decay back to ember via the ~0.92×/frame envelope.
- **D-02** — Per-edge flash fires in **LOCKED state ONLY**, never ACQUIRING. ACQUIRING
  owns the dedicated 2 Hz `blinkPhase` blink. Clean separation: 2 Hz blink = acquiring,
  per-edge flash = locked & responding.
- **D-03** — Peak intensity is **clearly visible / punchy**, distinct from steady-state
  at a glance. Exact peak magnitude (peak white-point + glow boost factor) is a
  single-constant tune to be finalized at the phase acceptance gate.

### Claude's Discretion
- Exact numeric constants (peak white-point, glow blur radius, brightness multiplier)
  left to implementation + acceptance gate.
- **Data-flow mechanism (planner's call):** lock-free signal from audio thread to
  `WaveformDisplay`. Suggested: `std::atomic<int>` edge counter incremented on each
  LOCKED clock edge at the `clockTrigger.process` site (line 446), watched by `step()`
  for changes to retrigger a decaying flash-intensity envelope. Exact field names/shape
  are the planner's to decide.

### Deferred Ideas (OUT OF SCOPE)
- Separate display pills from waveform visualiser — display architecture refactor.
- Surge-style modulation routing system — own milestone.
- Pulse width modulation — already delivered (Phase 18).
- Any change to BPM stack, ratio pill, border glow, or other display elements — SYNC
  badge ONLY.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ANIM-01 | SYNC badge flashes on each clock edge with bright pulse | Edge already detected at AnalogLFO.cpp:446 (`clockTrigger.process`). Add `std::atomic<int>` edge counter incremented when `clockState == LOCKED`; widget `step()` watches for change and resets a flash envelope to peak. Render via color-lerp + glow-bloom in the SYNC draw block (lines 1369–1380) through `drawPillText`. |
| ANIM-02 | Flash uses exponential decay (~0.92×/frame) back to steady state | Multiplicative decay `flash *= 0.92f` per `step()` call advances in `WaveformDisplay::step()` (line 868), mirroring the existing per-frame animation timers. Verified: 0.92^N reaches ~0.01 in ~55 frames ≈ 0.92s perceptually, with the visible portion (down to ~0.1) in ~28 frames ≈ 0.46s — see Pitfall 2 for the ≈200ms reconciliation. |
</phase_requirements>

## Summary

This is a small, self-contained DSP→widget animation against an existing, well-structured
C++/NanoVG codebase. **Every file/line reference in CONTEXT.md was verified accurate this
session** — line 446 (edge detection), 110–162 (display atomics), 868 (`step()`), 1161
(`drawPillText`), 1369–1380 (SYNC badge draw). The architecture for this feature already
exists in miniature: the codebase has an established lock-free DSP→GUI publishing pattern
(`std::atomic<...>` fields stored with `std::memory_order_relaxed` in `process()`, loaded
with the same ordering in the widget), an established per-frame animation convention in
`step()`, and a two-pass (glow + sharp) emissive text renderer in `drawPillText()` that
already draws on `drawLayer` layer 1 (the light/emissive layer). The new flash hooks cleanly
into all three.

The suggested mechanism is sound with one important subtlety: at audio rate, **multiple LOCKED
clock edges can occur between two `step()` calls is effectively impossible for musical clocks**
(a 1000 BPM clock at x16 ratio is ~266 edges/sec vs ~60 step/sec — see Pitfall 4), but the
**counter-delta** approach (store last-seen count, compare, reset envelope on any increase)
is still the correct design because it is naturally robust to missed/coalesced edges: the flash
simply re-peaks once per observed change, which is the desired visual. Do NOT use a boolean flag
that the widget clears — that creates a read-modify-write race; the audio thread must only ever
*increment* and the widget must only ever *read*.

The decay constant has a real frame-rate-independence concern (Pitfall 2): `0.92×/frame` is
locked by ANIM-02 and the existing codebase animations are likewise frame-count-based (not
time-based), so the new flash should match that convention for consistency — but note that VCV
Rack's widget `step()` runs at the UI frame rate (~60 Hz nominal, but variable), so at 120 Hz
the decay would be twice as fast in wall-clock time. The existing `breathePhase`/`blinkPhase`
animations share this exact characteristic, so matching them is the right call; flag the
assumption for the acceptance gate.

**Primary recommendation:** Add one `std::atomic<int> displayClockEdge{0}` field next to the
other display atomics (lines 110–162); `displayClockEdge.fetch_add(1, std::memory_order_relaxed)`
(or `.store(.load()+1)`) inside the `clockTrigger.process(...)` block at line 446 **guarded by
`clockState == LOCKED`**; in `WaveformDisplay`, add `int prevClockEdge = 0;` and
`float flashIntensity = 0.f;`, in `step()` detect the counter change to set `flashIntensity = 1.f`
then decay `flashIntensity *= 0.92f`; and in the SYNC draw block (1369–1380) pass `flashIntensity`
into an enhanced `drawPillText` (new optional param) that lerps text color ember→white-hot and
boosts glow blur radius + glow alpha proportionally to the intensity. Wire the flash into the
**LOCKED** branch only; leave the ACQUIRING `blinkPhase` branch untouched.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Detect clock edge (when LOCKED) | DSP / audio thread (`process`) | — | Edge already detected at audio rate via `dsp::SchmittTrigger` at line 446; the only authoritative place a beat occurs. |
| Publish edge signal to GUI | Lock-free atomic bridge | — | Mirrors existing `displayClockState` / `displayPhase` / `displaySmoothedPeriod` atomics — the established cross-thread boundary. |
| Advance/decay flash envelope | GUI / widget (`step()`) | — | Per-frame animation lives in `WaveformDisplay::step()` alongside `breathePhase`, `blinkPhase`, fade alphas. Frame-rate-paced, matching ANIM-02's per-frame decay. |
| Render flash (color + bloom) | GUI / widget (`drawLayer` layer 1) | — | Emissive text already drawn on layer 1 by `drawPillText` via `drawTextOverlays`; the flash modulates that existing two-pass render. |

**Key boundary rule:** the audio thread only ever *writes/increments* the atomic; the widget
only ever *reads* it and owns all envelope/animation state. No shared mutable animation state
crosses the thread boundary.

## Standard Stack

No new external dependencies. This phase is pure in-repo C++ against the already-linked
VCV Rack SDK and bundled NanoVG.

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | `../Rack-SDK` (relative, per build memory) | Module/widget framework, `dsp::SchmittTrigger`, `drawLayer` light layer | Already the project's only framework. |
| NanoVG | bundled with Rack SDK | Vector text/shape rendering (`nvgFontBlur`, `nvgFillColor`, `nvgRGBAf`, `nvgText`) | Already used throughout `WaveformDisplay`. |
| `<atomic>` | C++ stdlib (already `#include`d, line 3) | Lock-free DSP→GUI signal | Established pattern — 25 `memory_order_relaxed` uses already in file. |
| `<cmath>` | C++ stdlib (already `#include`d, line 2) | `std::sin`, lerp arithmetic; no new math needed | Already present. |

### Supporting
None. No `<chrono>`, no timers, no new headers required.

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `std::atomic<int>` edge counter | `std::atomic<bool>` flag set by audio / cleared by widget | REJECT: widget clearing the flag is a read-modify-write the audio thread also writes → lost-wakeup race. Counter is monotonic, single-writer. |
| Per-frame `*= 0.92f` decay | Time-based `exp(-dt/tau)` decay (frame-rate-independent) | Time-based is more correct in wall-clock terms, BUT ANIM-02 locks `0.92×/frame` AND existing animations (`breathePhase`, `blinkPhase`, fade alphas) are all frame-count-based. Match the codebase + requirement. |
| New `flashIntensity` param on `drawPillText` | Separate `drawFlashingPillText` function | A defaulted param (`float flash = 0.f`) keeps one renderer, zero behavior change for the 6 other callers. |

**Installation:** none.

## Package Legitimacy Audit

Not applicable — this phase installs **no external packages**. All code uses the
already-linked VCV Rack SDK, bundled NanoVG, and C++ standard library headers already
`#include`d in `src/AnalogLFO.cpp` (`<atomic>`, `<cmath>`, `<array>`). slopcheck / registry
verification not run because nothing is being installed.

## Architecture Patterns

### System Architecture Diagram

```
  CLK input voltage
        │
        ▼
  ┌─────────────────────────────────────────────┐
  │ AnalogLFO::process() — AUDIO THREAD          │
  │                                              │
  │  clockTrigger.process(clk, 0.1, 1.0) @446 ──┐│
  │                                            ││ rising edge
  │   (existing: EMA period, state machine)   ▼│
  │                              if clockState == LOCKED:
  │                                 displayClockEdge.fetch_add(1, relaxed)  ◄── NEW
  │                                 displayClockState.store(LOCKED, relaxed) (existing)
  └─────────────────────────────────│───────────┘
                                     │  atomic<int> (lock-free bridge, lines 110-162)
                                     ▼
  ┌─────────────────────────────────────────────┐
  │ WaveformDisplay::step() — UI THREAD (~60Hz)  │
  │                                              │
  │  edge = displayClockEdge.load(relaxed)       │  ◄── NEW
  │  if (edge != prevClockEdge) {                │
  │      flashIntensity = 1.f;  // re-peak       │
  │      prevClockEdge = edge;                   │
  │  }                                           │
  │  flashIntensity *= 0.92f;   // ANIM-02 decay │  ◄── NEW
  │  (existing: breathePhase, blinkPhase, fades) │
  └─────────────────────────────────│───────────┘
                                     │  float flashIntensity (widget-owned)
                                     ▼
  ┌─────────────────────────────────────────────┐
  │ drawLayer(layer==1) → drawTextOverlays()     │
  │   → SYNC badge block (1369-1380)             │
  │                                              │
  │   clockState = displayClockState.load @1332  │  (already loaded)
  │   if syncFadeAlpha > 0.001:                  │
  │     if ACQUIRING: alpha *= blink (untouched) │
  │     else (LOCKED): pass flashIntensity ──────┼──► drawPillText(..., flash)
  │                                              │      • color lerp ember → white-hot
  │                                              │      • glow blur 2.0 → ~2.0+k·flash
  │                                              │      • glow alpha 0.3 → 0.3+k·flash
  └──────────────────────────────────────────────┘
```

### Recommended Project Structure
Single file — no new files. All edits in `src/AnalogLFO.cpp`:

```
src/AnalogLFO.cpp
├── struct AnalogLFO (module)
│   ├── ~line 128-162  display* atomics      ← ADD: std::atomic<int> displayClockEdge{0};
│   └── ~line 446      clockTrigger.process  ← ADD: LOCKED-guarded fetch_add
└── struct WaveformDisplay (widget)
    ├── ~line 849-866  animation state        ← ADD: int prevClockEdge; float flashIntensity;
    ├── ~line 868      step()                 ← ADD: edge-delta detect + 0.92× decay
    ├── ~line 1161     drawPillText()         ← ADD: optional float flash param (color+bloom)
    └── ~line 1369-80  SYNC badge block       ← EDIT: pass flashIntensity in LOCKED branch
```

### Pattern 1: Lock-free DSP→GUI atomic publish
**What:** Audio thread stores; UI thread loads; both use `std::memory_order_relaxed`.
**When to use:** Any value crossing the audio→GUI boundary in this codebase.
**Example (existing, verified):**
```cpp
// Source: src/AnalogLFO.cpp:128, :458, :1332 (existing displayClockState)
// Declaration (member of AnalogLFO):
std::atomic<int> displayClockState{0};
// Audio thread write (in process / handleClockInput):
displayClockState.store(LOCKED, std::memory_order_relaxed);
// UI thread read (in drawTextOverlays / step):
int clockState = module->displayClockState.load(std::memory_order_relaxed);
```
New field mirrors this exactly: `std::atomic<int> displayClockEdge{0};` written with
`fetch_add(1, std::memory_order_relaxed)` (or `store(load(relaxed)+1, relaxed)`), read with
`load(std::memory_order_relaxed)`.

> Note: most display atomics use `relaxed`; the double-buffer index `displayReadIdx` uses
> `acquire`/`release` (lines 410-ish / 1424) because it gates access to a shared buffer.
> The edge counter does NOT gate a buffer — `relaxed` is correct and matches `displayClockState`.

### Pattern 2: Per-frame animation envelope in step()
**What:** Animation state advanced once per `step()` call (frame-paced), wrapped/clamped.
**When to use:** All widget animation in this codebase.
**Example (existing, verified):**
```cpp
// Source: src/AnalogLFO.cpp:870-895
breathePhase += 2.f * (float)M_PI * 0.2f / 60.f;       // 0.2 Hz, assumes 60fps
blinkPhase   += 2.f * (float)M_PI * 2.f / 60.f;        // 2 Hz, assumes 60fps
float fadeSpeed = 1.f / (0.2f * 60.f);                 // 200ms linear fade @ 60fps
syncFadeAlpha += rack::math::clamp(syncTarget - syncFadeAlpha, -fadeSpeed, fadeSpeed);
```
New flash decay follows the same per-frame, 60fps-assumed convention:
```cpp
// In step(), after the existing fade block:
int edge = module->displayClockEdge.load(std::memory_order_relaxed);
if (edge != prevClockEdge) { flashIntensity = 1.f; prevClockEdge = edge; }
flashIntensity *= 0.92f;                               // ANIM-02 locked decay
if (flashIntensity < 0.001f) flashIntensity = 0.f;     // snap to steady state
```

### Pattern 3: Two-pass emissive text (glow + sharp)
**What:** A blurred low-alpha glow pass, then a sharp full-alpha pass, in the same ember color.
**When to use:** Every pill in the display.
**Example (existing, verified):**
```cpp
// Source: src/AnalogLFO.cpp:1188-1196
// Glow text pass
nvgFontBlur(vg, 2.0f);
nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha * 0.3f));
nvgText(vg, x, y, text, NULL);
// Sharp text pass
nvgFontBlur(vg, 0.0f);
nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha));
nvgText(vg, x, y, text, NULL);
```
Flash treatment modulates BOTH passes by a `flash` value (0..1):
```cpp
// Color: lerp ember -> white-hot (target ~1.0/0.95/0.7 = hot white-yellow; tune at gate)
float r = 0.91f  + flash * (1.00f - 0.91f);
float g = 0.365f + flash * (0.95f - 0.365f);
float b = 0.149f + flash * (0.70f - 0.149f);
// Glow bloom: bigger blur + stronger glow alpha at peak (constants are the acceptance-gate tune)
float glowBlur  = 2.0f + flash * 3.0f;        // 2.0 steady -> ~5.0 peak
float glowAlpha = alpha * (0.3f + flash * 0.7f);
nvgFontBlur(vg, glowBlur);
nvgFillColor(vg, nvgRGBAf(r, g, b, glowAlpha));
nvgText(vg, x, y, text, NULL);
nvgFontBlur(vg, 0.0f);
nvgFillColor(vg, nvgRGBAf(r, g, b, alpha));    // sharp pass also takes the hot color
nvgText(vg, x, y, text, NULL);
```
At `flash == 0` this is byte-for-byte the existing steady-state render — zero regression for
the badge at rest and zero change for the other six `drawPillText` callers (defaulted param).

### Anti-Patterns to Avoid
- **Boolean flag cleared by the widget:** `std::atomic<bool> flash` set by audio, reset by
  widget → both threads write → race / lost flashes. Use a monotonic counter; widget only reads.
- **Branching the flash inside ACQUIRING:** D-02 forbids it. Keep the flash strictly in the
  `else`/LOCKED path of the existing `if (clockState == ACQUIRING)` at line 1373.
- **Driving brightness via alpha alone:** steady state is already `alpha == 1.0` (line 1195),
  so alpha cannot exceed it — the flash MUST come from color shift + glow bloom (D-01).
- **Resetting the envelope in `process()`:** all animation state belongs to the widget; the
  audio thread must not touch `flashIntensity`.
- **Reading the counter in `drawLayer` instead of `step()`:** `drawLayer` can be called more
  or fewer times than `step()`; advance the envelope exactly once per `step()` (matches existing
  animations) and only *read* `flashIntensity` in the draw path.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Cross-thread signal | Custom mutex / ring buffer | `std::atomic<int>` (existing pattern) | Audio thread must never block; counter is single-writer lock-free. |
| Text glow / bloom | Manual multi-sample offset blur | `nvgFontBlur()` (already used) | NanoVG renders blurred text natively; existing code already does the two-pass technique. |
| Edge detection | New rising-edge logic | Existing `clockTrigger.process()` @446 | The edge is already detected for clock tracking; just hook the counter onto it. |
| Color interpolation | Color-space lib | Plain linear lerp on the three `nvgRGBAf` floats | Already how the file expresses all colors; sRGB-linear nuance is irrelevant for a hot-flash tint. |

**Key insight:** This feature is 95% wiring into existing, proven mechanisms. The only genuinely
new code is ~3 lines in `process()`, ~5 lines in `step()`, and a color/blur modulation inside
the already-existing two-pass text render.

## Runtime State Inventory

> This is an in-process, in-memory animation. No persisted/runtime state crosses a process,
> OS, or storage boundary. Still inventoried explicitly per protocol:

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — flash state is transient widget memory, never serialized. Verified: no `dataToJson`/`dataFromJson` touch animation fields. | None |
| Live service config | None — no external service involved. | None |
| OS-registered state | None — no OS registration; pure plugin render. | None |
| Secrets/env vars | None. | None |
| Build artifacts | The compiled `plugin.dylib` + the **extracted** install dir `~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/` can hold a STALE dylib. After `make install`, copy the fresh dylib into the extracted dir and **fully quit + relaunch Rack** (per build memory). | Reinstall + relaunch on every build during UAT |

## Common Pitfalls

### Pitfall 1: Widget clears the signal → cross-thread race
**What goes wrong:** Using `atomic<bool>` that the widget resets to consume the edge; the audio
thread sets it concurrently, so a flash is silently lost or double-fired.
**Why it happens:** Treating the atomic as a one-shot mailbox both sides write.
**How to avoid:** Monotonic `int` counter — audio increments only, widget compares against a
**widget-local** `prevClockEdge` and never writes the atomic.
**Warning signs:** Occasional missed flashes under fast clocks; flicker that depends on timing.

### Pitfall 2: `0.92×/frame` is frame-rate dependent and the ≈200ms is approximate
**What goes wrong:** ANIM-02 says "≈200ms," but `0.92^N` does NOT reach a negligible threshold
in 200ms at 60fps. `0.92^N`: at N=12 frames (200ms@60fps) intensity = `0.92^12 ≈ 0.37` — still
clearly visible. It reaches ~0.1 at N≈28 (~0.46s) and ~0.01 at N≈55 (~0.92s). So the *perceived*
"mostly faded" window is roughly 250–450ms, and the long tail is sub-perceptual. **This is fine
and on-theme** (a cooling ember tails off), but the planner/acceptance-gate should treat "≈200ms"
as the time-to-mostly-faded, not time-to-zero. Additionally, `step()` is paced by the UI frame
rate, which VCV Rack does not guarantee at exactly 60 Hz — at 120 Hz the decay is twice as fast
in wall-clock time.
**Why it happens:** Multiplicative decay has a long exponential tail; frame-count decay scales
with refresh rate.
**How to avoid:** Match the existing codebase convention (all animations are frame-count / 60fps-
assumed — `breathePhase`, `blinkPhase`, `fadeSpeed`), accept the locked `0.92×/frame`, and snap
`flashIntensity` to `0` below a small threshold (e.g. `< 0.001`) to avoid a perpetual tiny tail.
Flag the wall-clock-vs-refresh-rate behavior at the acceptance gate (it matches every other
animation in the module, so this is consistent, not a defect). **[ASSUMED: A1]** that matching the
existing 60fps-frame-count convention is preferred over a time-based `dt` decay — confirm at gate.

### Pitfall 3: Hot color must apply to BOTH text passes, not just the glow
**What goes wrong:** Only blooming the glow pass while the sharp pass stays ember → the badge
reads as a fuzzy ember halo, not "white-hot," failing D-03's "clearly brighter."
**Why it happens:** The glow pass is visually dominant in bloom but the sharp pass carries the
legible core; leaving it ember undercuts the white-hot read.
**How to avoid:** Apply the lerped hot color to both the glow and sharp `nvgFillColor` calls
(see Pattern 3). Optionally leave the pill fill/stroke (lines 1180/1184) ember — D-01 scopes the
flash to "text/glow," so the rounded-rect background can stay constant.

### Pitfall 4: Assuming many edges per frame need queuing
**What goes wrong:** Over-engineering a queue/ring buffer to capture every edge between frames.
**Why it happens:** Audio runs at 44.1k+ Hz; instinct says edges pile up.
**How to avoid:** Clock edges are musical-rate, not audio-rate. Worst realistic case: ~300 BPM
× x16 ratio ≈ 80 edges/sec, still slower than ~60 step/sec for most cases and even at parity the
desired visual is "re-peak on each observed change" — the counter-delta handles coalescing
gracefully (if two edges land in one frame, you get one re-peak, which is correct: the badge is
already at peak). No queue needed.

### Pitfall 5: Editing the ACQUIRING branch breaks D-02 separation
**What goes wrong:** Tucking the flash into the wrong side of the `if (clockState == ACQUIRING)`
at line 1373, causing the flash to fire during acquisition or the blink to fight it.
**How to avoid:** The branch is already clean (verified line 1373–1376). Flash goes in the
implicit `else` (LOCKED). `clockState` is already loaded at line 1332 and in scope — no extra read.
Keep `blinkPhase` untouched.

### Pitfall 6: Stale install masks the change during UAT
**What goes wrong:** Build succeeds, Rack shows old behavior (no flash). Per build memory, the
extracted plugin dir keeps a stale dylib and Rack caches at load.
**How to avoid:** After `make install RACK_DIR=../Rack-SDK`, copy the fresh `plugin.dylib` into
the extracted `ForgeAudio-AnalogSeries/` dir and fully quit + relaunch Rack. Do NOT run executors
in a git worktree — `../Rack-SDK` won't resolve and the compile gate fails (build memory).

## Code Examples

### Declaring the edge counter (next to existing display atomics)
```cpp
// Source pattern: src/AnalogLFO.cpp:128 (displayClockState)
std::atomic<int> displayClockEdge{0};   // incremented once per LOCKED clock edge (ANIM-01)
```

### Incrementing on a LOCKED edge (inside the existing edge block)
```cpp
// Source site: src/AnalogLFO.cpp:446, inside if (clockTrigger.process(clkVoltage, 0.1f, 1.0f))
// Place AFTER the state machine has potentially set LOCKED this edge, OR gate on current state.
// Simplest correct placement: at the very end of the edge block, guarded:
if (clockState == LOCKED) {
    displayClockEdge.fetch_add(1, std::memory_order_relaxed);
}
```
> Planner decision: whether to count the edge that *transitions into* LOCKED. Recommended: only
> flash on edges where `clockState == LOCKED` is already true at the increment site, so the very
> first locked edge flashes on the *next* beat — visually clean (no flash mid-acquisition). The
> increment must be placed where `clockState` already reflects this edge's transition (after the
> ACQUIRING→LOCKED promotion at lines 482/496).

### Widget state + envelope (step)
```cpp
// Source pattern: src/AnalogLFO.cpp:849-866 (animation state), :868-895 (step body)
// Member fields:
int   prevClockEdge = 0;
float flashIntensity = 0.f;
// In step(), within the existing `if (module) { ... }` block:
int edge = module->displayClockEdge.load(std::memory_order_relaxed);
if (edge != prevClockEdge) { flashIntensity = 1.f; prevClockEdge = edge; }
flashIntensity *= 0.92f;                       // ANIM-02
if (flashIntensity < 0.001f) flashIntensity = 0.f;
```

### Applying the flash in the SYNC badge block
```cpp
// Source site: src/AnalogLFO.cpp:1369-1380
if (syncFadeAlpha > 0.001f) {
    float effectiveAlpha = syncFadeAlpha;
    if (clockState == AnalogLFO::ACQUIRING) {
        float blink = 0.5f + 0.5f * std::sin(blinkPhase);
        effectiveAlpha *= blink;
        drawPillText(vg, font->handle, rightColX, topY + pillLabelSize,
                     "SYNC", pillLabelSize, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM,
                     effectiveAlpha /* , 0.f flash */);
    } else {  // LOCKED — per-edge flash (D-02)
        drawPillText(vg, font->handle, rightColX, topY + pillLabelSize,
                     "SYNC", pillLabelSize, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM,
                     effectiveAlpha, flashIntensity);
    }
}
```
With `drawPillText` signature extended to `..., float alpha, float flash = 0.f)` and Pattern 3's
color/blur modulation applied to its two text passes. The default keeps all other callers
(lines 1226, 1345, 1352, 1361, BPM stack) unchanged.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| n/a — first animated badge | `drawLayer` light layer (layer 1) for emissive content | Established Phase 20 | Flash correctly belongs on layer 1 (already where `drawTextOverlays` runs, line 1433) so the bloom reads as emitted light, not flat fill. |

**Deprecated/outdated:** none relevant. NanoVG `nvgFontBlur` is the current, supported text-blur
API in the bundled NanoVG; the codebase already relies on it (lines 1151, 1189).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Per-frame `0.92×` decay (frame-count, 60fps-assumed) is preferred over time-based `dt` decay, matching existing animations. ANIM-02 locks `0.92×/frame` so this is near-certain, but the 60fps assumption (vs variable refresh) is unverified against Rack's actual step cadence. | Pitfall 2 / Pattern 2 | At very high refresh rates the flash decays faster in wall-clock time. Matches every other animation in the module, so consistent; confirm "feels right" at the visual acceptance gate. |
| A2 | White-hot target color (~1.0/0.95/0.7) and bloom constants (blur 2→5, glow alpha 0.3→1.0) are starting points only. | Pattern 3 / Code Examples | Purely cosmetic; D-03 explicitly defers exact constants to the acceptance gate. Zero structural risk. |
| A3 | Counting only edges where `clockState == LOCKED` is already true (first locked beat flashes on the following edge) is the cleanest visual. | Code Examples | Could debate flashing on the transition edge; trivial one-line placement change. Confirm at gate. |

**Note:** All file:line references, the atomic publish pattern, the `step()` animation
convention, and the two-pass text render were VERIFIED directly against `src/AnalogLFO.cpp`
this session — those are not assumptions.

## Open Questions

1. **Flash on the LOCKED-transition edge, or only subsequent LOCKED edges?**
   - What we know: The state machine promotes to LOCKED at lines 482/496 inside the edge block.
   - What's unclear: Whether the user wants the very first locked beat to flash.
   - Recommendation: Increment only when `clockState == LOCKED` already (subsequent beats);
     visually avoids a flash colliding with the acquiring→locked handoff. One-line change to
     revisit at the gate if the user wants the first beat to pop.

2. **Should the pill background/stroke also warm slightly, or text/glow only?**
   - What we know: D-01 says "badge text/glow shifts… and the existing glow text pass blooms."
   - What's unclear: D-01 scopes it to text/glow; the rounded-rect fill (line 1180) is not mentioned.
   - Recommendation: Keep fill/stroke ember (constant); flash text + glow only. Easy to extend
     to the pill if the gate wants more punch.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| VCV Rack SDK (`../Rack-SDK`) | Compile/link | Assumed ✓ (used every prior phase) | per project | — (worktree breaks path; build on main tree) |
| `make` toolchain (clang) | Compile gate | Assumed ✓ | — | — |
| VCV Rack app (for UAT) | Visual acceptance | Assumed ✓ (manual UAT) | Rack 2 (plugins-mac-arm64) | — |
| slopcheck / npm / pip | — | Not needed | — | N/A — no packages installed |

**Missing dependencies with no fallback:** none identified (all build deps used in prior phases).
**Missing dependencies with fallback:** none.
> Build/install caveat (from project memory): run on the **main tree, not a worktree**
> (`../Rack-SDK` relative path); after install, sync the extracted dir + relaunch Rack.

## Validation Architecture

> `workflow.nyquist_validation` is absent from `.planning/config.json` → treated as enabled.
> However, this project has **NO automated test suite** (confirmed: no `test/`, `tests/`, or
> test files; build memory states "there is NO automated test suite; the compile gate is the only
> automated signal, all alignment/regression is manual in-Rack UAT with screenshots"). The
> standard test-framework table does not apply.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None — no unit/integration test harness exists for this VCV plugin |
| Config file | none |
| Quick run command | `make RACK_DIR=../Rack-SDK` (compile gate — the only automated signal) |
| Full suite command | `make RACK_DIR=../Rack-SDK && make install RACK_DIR=../Rack-SDK` then manual in-Rack UAT |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ANIM-01 | SYNC badge flashes on each LOCKED clock edge | manual-only (visual) | `make RACK_DIR=../Rack-SDK` (compiles); flash verified visually in Rack with a clock source | n/a — visual |
| ANIM-02 | Flash decays smoothly ~200ms, no abrupt transition | manual-only (visual) | compile gate; decay smoothness judged visually / via screen capture | n/a — visual |

### Sampling Rate
- **Per task commit:** `make RACK_DIR=../Rack-SDK` must compile clean (zero warnings ideally).
- **Per wave merge:** compile + `make install` + relaunch Rack, spot-check badge unchanged at rest.
- **Phase gate:** Full visual UAT — patch a clock into CLK, confirm (1) flash visible & distinct
  from steady state in LOCKED, (2) smooth ~200ms decay no strobe, (3) ACQUIRING still blinks at
  2 Hz with NO per-edge flash, (4) no flash when free-running.

### Wave 0 Gaps
- None — no test infrastructure to create (project is intentionally compile-gate + manual UAT).
  Implementation tasks should each end with a clean compile; the phase ends with a human visual
  acceptance gate (the locked tuning constants are decided there).

## Security Domain

> `security_enforcement` not present in config. This phase has no security surface: no I/O, no
> network, no untrusted input, no auth, no crypto, no persisted/serialized data. The only input
> is a clock voltage already validated by the existing `SchmittTrigger`. ASVS categories
> (V2–V6) do not apply to an in-process render animation. No threat patterns relevant.

## Sources

### Primary (HIGH confidence)
- `src/AnalogLFO.cpp` (read this session) — verified: edge detection block (440–535), display
  atomics (107–162), `WaveformDisplay::step()` (868–905), `drawPillText()` (1161–1197),
  `drawTextOverlays()`/SYNC badge block (1314–1386), `drawLayer()` (1407–1444). All CONTEXT.md
  line references confirmed accurate.
- `.planning/phases/21-animated-sync-badge/21-CONTEXT.md` — locked decisions D-01..D-03.
- `.planning/REQUIREMENTS.md` §Animation — ANIM-01, ANIM-02.
- Project memory `vcv_build_install_workflow.md` — build/install/stale-flush, no test suite,
  worktree caveat.
- Project memory `project_design_language.md` — Forge Noir ember `#e85d26`, restrained aesthetic.

### Secondary (MEDIUM confidence)
- NanoVG `nvgFontBlur` semantics (blur radius in px, larger = more bloom) — corroborated by the
  codebase's own usage (`2.0f` glow vs `0.0f` sharp, lines 1151/1156/1189/1194).

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new deps; all APIs already used in-file and verified.
- Architecture: HIGH — mirrors three existing, verified patterns (atomic publish, per-frame
  envelope, two-pass text). Mechanism is the CONTEXT.md suggestion, validated as sound.
- Pitfalls: HIGH — derived from direct code reading and the locked requirements; the decay-math
  reconciliation (Pitfall 2) computed explicitly.

**Research date:** 2026-06-13
**Valid until:** stable — 30 days (in-repo C++ against a pinned SDK; the only volatility is the
file line numbers if the file is edited before planning, so re-grep anchors before implementing).
