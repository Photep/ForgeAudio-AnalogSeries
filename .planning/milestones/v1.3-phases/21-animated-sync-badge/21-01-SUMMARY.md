---
phase: 21-animated-sync-badge
plan: 01
subsystem: ui
tags: [vcv-rack, nanovg, c++, lock-free-atomic, animation, sync-badge]

# Dependency graph
requires:
  - phase: 20-display-layout-crt-aesthetic
    provides: drawPillText two-pass emissive text renderer, ember palette, drawTextOverlays
  - phase: 20.1-panel-redesign-18hp-fresh-layout
    provides: final 18HP SYNC badge position/size, blinkPhase 2Hz ACQUIRING blink
provides:
  - std::atomic<int> displayClockEdge edge counter (LOCKED-gated, audio write-only)
  - WaveformDisplay flash envelope (flashIntensity, prevClockEdge) with 0.92x/frame decay
  - flash-modulated drawPillText (defaulted float flash param; ember->white-hot lerp + glow bloom)
  - SYNC LOCKED-branch wiring (per-edge white-hot flash; ACQUIRING blink untouched)
affects: [future-display-animations, sync-badge-tuning]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Lock-free DSP->GUI edge signal via monotonic atomic<int> counter (single-writer audio increments, single-reader widget compares against widget-local prevClockEdge)"
    - "Per-frame exponential flash envelope advanced once per step(), re-armable to peak mid-decay"
    - "Defaulted flash param on shared renderer (drawPillText) so all other callers are byte-for-byte unchanged"

key-files:
  created: []
  modified: [src/AnalogLFO.cpp]

key-decisions:
  - "[21-01]: Edge counter increments only when clockState == LOCKED already at the increment site (first locked beat flashes on the NEXT edge -- clean acquiring->locked handoff, no collision; per Assumption A3 / Open Question 1)"
  - "[21-01]: Flash drives text color + glow bloom on BOTH passes; sharp-pass alpha stays full (brightness from color+glow not alpha, D-01)"
  - "[21-01]: Starting peak constants per UI-SPEC -- white-hot (1.0, 0.93, 0.78), peak blur 5.0, peak glow alpha 0.9; final magnitude deferred to the human acceptance gate (D-03)"

patterns-established:
  - "Monotonic atomic counter for cross-thread edge signalling (never a bool mailbox the widget clears -- avoids read-modify-write race)"
  - "Flash envelope re-peaks on observed counter change, naturally coalescing multiple edges per frame"

requirements-completed: [ANIM-01, ANIM-02]  # verified at the Task 4 human-verify gate (approved 2026-06-13, fresh dylib loaded after stale-install flush)

# Metrics
duration: ~7min (Tasks 1-3; Task 4 human-verify pending)
completed: 2026-06-13
---

# Phase 21 Plan 01: Animated SYNC Badge Summary

**Per-edge white-hot SYNC badge flash while LOCKED, driven by a lock-free atomic edge counter and a 0.92x/frame exponential decay envelope; ember-to-white-hot color lerp plus glow bloom on both NanoVG text passes. Code + compile gate complete; in-Rack visual acceptance is the pending human gate.**

## Status: COMPLETE — all 4 tasks done (Task 4 human-verify APPROVED 2026-06-13)

> **Task 4 resolution:** The initial in-Rack UAT reported ANIM-01/ANIM-02 as failing (no flash). Root cause was a **stale install**, not a code defect — Rack was loading the Jun-12 Phase-20.1 `plugin.dylib` (hash `622d92e`), not the fresh Phase-21 build (hash `9d47a77`). After syncing the fresh dylib into the extracted plugins dir and fully relaunching Rack, the per-edge white-hot flash was confirmed and **approved with no peak-constant tuning changes** (UI-SPEC starting values accepted).


Tasks 1, 2, and 3 are implemented, committed atomically, and pass the compile gate (`make RACK_DIR=../Rack-SDK` exits 0, produces `plugin.dylib`). Task 4 is a `checkpoint:human-verify` gate (`gate="blocking"`) — install + in-Rack visual UAT + peak-constant tuning — and has been intentionally handed back to the orchestrator for the human. It was NOT auto-completed: no `make install`, no Rack relaunch, no self-approval of the visual UAT.

## Performance

- **Duration:** ~7 min (Tasks 1-3)
- **Started:** 2026-06-13T17:06 (approx, first task commit)
- **Completed (Tasks 1-3):** 2026-06-13T17:08:31+10:00
- **Tasks:** 3 of 4 (Task 4 = pending human-verify gate)
- **Files modified:** 1 (`src/AnalogLFO.cpp`)

## Accomplishments
- **Task 1 (DSP):** Added `std::atomic<int> displayClockEdge{0}` beside the existing `display*` atomics; `fetch_add(1, relaxed)` once per clock edge, guarded by `clockState == LOCKED`, placed after the ACQUIRING->LOCKED promotion. Audio thread is write-only on the counter.
- **Task 2 (widget + render):** Added `prevClockEdge` + `flashIntensity` to `WaveformDisplay`; `step()` re-arms `flashIntensity = 1.f` on counter change, decays `*= 0.92f` per frame, snaps to 0 below `0.001`. Extended `drawPillText` with a trailing defaulted `float flash = 0.f` that lerps ember->white-hot color on BOTH passes and blooms glow blur (2->5) and glow alpha (0.3->0.9). At `flash == 0` the render is byte-for-byte the prior steady state.
- **Task 3 (wiring + gate):** Wired `flashIntensity` into the SYNC LOCKED (non-ACQUIRING) branch only; ACQUIRING 2Hz blink left byte-for-byte unchanged (D-02). Compile gate passes clean on the main tree, producing `plugin.dylib`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add LOCKED-gated edge counter (DSP side)** - `771ed94` (feat)
2. **Task 2: Add flash envelope (step) + flash-modulated drawPillText (render)** - `92f779e` (feat)
3. **Task 3: Wire flash into SYNC LOCKED branch + compile gate** - `413081f` (feat)

**Task 4: Install + in-Rack visual acceptance gate** — PENDING (human-verify checkpoint, not committed; see "Pending Human-Verify Gate" below).

## Files Created/Modified
- `src/AnalogLFO.cpp` — added `displayClockEdge` atomic + LOCKED-guarded `fetch_add` in the edge block; added `prevClockEdge`/`flashIntensity` widget state + per-frame envelope in `step()`; extended `drawPillText` with the defaulted `flash` param and the two-pass color/glow modulation; wired `flashIntensity` into the SYNC LOCKED branch.

## Decisions Made
- Counted only edges where `clockState == LOCKED` is already true at the increment site (first locked beat flashes on the next edge) — cleanest visual, no flash colliding with the acquiring->locked handoff (Assumption A3 / Open Question 1).
- Applied the lerped hot color to BOTH the glow and sharp text passes (Pitfall 3); kept the sharp-pass alpha at full `alpha` so brightness comes from color + glow, not alpha (D-01).
- Used the UI-SPEC starting peak constants (white-hot `1.0, 0.93, 0.78`, peak blur `5.0`, peak glow alpha `0.9`); exact magnitude deferred to the human acceptance gate (D-03).
- Kept the pill fill/stroke ember/unchanged — flash scoped to text + glow only (D-01 / Open Question 2 recommendation).

## Deviations from Plan

None - plan executed exactly as written.

(Note: the Task 2 automated verify line used a greedy `.*` regex across the two-line `drawPillText` signature and produced a false negative after `tr -d '\n'`; a targeted `[^)]*` grep confirmed the signature `..., float alpha, float flash = 0.f)` is exactly correct. No code change required — the implementation matches the acceptance criteria.)

## Issues Encountered
None.

## Pending Human-Verify Gate (Task 4)

This is a `checkpoint:human-verify` gate (`gate="blocking"`). The human must install the freshly built plugin, run the in-Rack visual UAT, and tune the peak constants. Resume instructions:

1. Install on the MAIN tree (per build memory): `make install RACK_DIR=../Rack-SDK`, then sync the fresh `plugin.dylib` into the extracted install dir `~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/`, and FULLY QUIT + relaunch Rack (stale-install flush, Pitfall 6).
2. Patch a clock source (CLOCK / square LFO) into CLK; let it lock.
3. Confirm (#1): once LOCKED, the SYNC badge visibly FLASHES white-hot on each clock edge, clearly distinct from steady ember (ANIM-01).
4. Confirm (#2): the flash decays smoothly back to ember over ~200ms, no abrupt strobe (ANIM-02).
5. Confirm D-02: while ACQUIRING the badge BLINKS at 2Hz with NO per-edge flash; free-running shows no flash.
6. Confirm scope: no other display element changed (BPM stack, ratio/Hz/swing pills, border glow, scanlines, corner brackets).
7. TUNE (D-03): if too weak/harsh, adjust ONLY the gate-tuned constants in `drawPillText` — white-hot hue `(1.0, 0.93, 0.78)`, peak blur (~4-5), peak glow alpha (~0.6-0.9). Do NOT change the `0.92f` decay or the LOCKED-only trigger. Rebuild/reinstall/relaunch/recheck.

**Resume signal:** Type "approved" (optionally noting the final tuned peak constants), or describe what looks wrong so it can be fixed.

## Next Phase Readiness
- ANIM-01 and ANIM-02 are implemented and compile clean; final acceptance (and any peak-constant tuning) is gated on the human visual UAT above.
- Phase 21 is the last plan of the phase; once the human gate is approved, the phase is ready for `/gsd:verify-work`.

## Self-Check: PASSED

- FOUND: `.planning/phases/21-animated-sync-badge/21-01-SUMMARY.md`
- FOUND: `src/AnalogLFO.cpp`
- FOUND: `plugin.dylib` (compile gate artifact)
- FOUND: commits 771ed94, 92f779e, 413081f, 8327d8c

---
*Phase: 21-animated-sync-badge*
*Tasks 1-3 completed: 2026-06-13 (Task 4 human-verify pending)*
