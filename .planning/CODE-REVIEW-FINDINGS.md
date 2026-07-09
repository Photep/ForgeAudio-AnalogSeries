# Code Review Findings — 2026-06-10

Full-codebase review of `src/AnalogLFO.cpp` and release packaging.
Line numbers refer to the working tree at commit `79be789` **plus the uncommitted
20-03 visual tweaks** (scanline rewrite, glow boost, zero-crossing removal).

Status legend: `[ ]` open · `[x]` fixed · `[-]` won't fix

---

## High priority — functional bugs

### [ ] 1. Clock tracker locks out permanently on >3× tempo jump

**Where:** `processClockInput()`, outlier rejection — `src/AnalogLFO.cpp:465-471`

**Problem:** Outlier rejection discards any edge whose period is >3× or <1/3 of
`smoothedPeriod` — but `clockTimer.reset()` has already run (line ~448) before the
check. If the incoming clock speeds up by more than 3× (e.g. clock source switched
from 1 PPQN to 4 PPQN, or patched through a sequential switch), every subsequent
edge is rejected as an outlier AND every edge resets the timer, so the no-pulse
timeout never fires. Module stays LOCKED at the stale tempo forever; only recovery
is unplugging the cable.

Slowdowns mostly recover via timeout, but fast clocks (smoothed period < ~0.33s)
have a narrow slowdown band with the same trap, because the timeout floor is 1s
(`timeout = max(1.0, min(3.0 * smoothedPeriod, 5.0))`).

**Repro:** Lock to a 120 BPM clock, switch the clock source to 4× the rate.
LFO never re-locks; SYNC stays lit at the old tempo.

**Proposed fix:** Count consecutive outliers. After 2-3 in a row, treat as a
genuine tempo change: snap `smoothedPeriod` to the new raw measurement (or drop
to ACQUIRING and re-learn). Reset the counter on any accepted edge.

---

### [ ] 2. x1.5 and /1.5 ratios truncate the waveform mid-cycle on every reset

**Where:** Division-aware phase reset — `src/AnalogLFO.cpp:519-523`,
ratio table at `src/AnalogLFO.cpp:43-59`

**Problem:** Reset logic only special-cases ratios < 1, using
`divisor = round(1/ratio)`. The two non-integer ratios fall through:

- **x1.5** (ratio 1.5): `shouldReset` is true on *every* clock edge, but the LFO
  completes 1.5 cycles per clock — every edge force-resets from phase 0.5 back
  to 0, chopping half a cycle every beat. (Integer multipliers are fine: phase is
  already ≈0 at each edge, reset is benign drift correction.)
- **/1.5** (ratio 2/3): `round(1/(2/3)) = round(1.5) = 2` → resets every 2 beats,
  but 2 clock beats = 1.333 LFO cycles — truncated a third of the way in.
  Correct alignment is every **3** beats (= 2 full cycles).

The 3ms cosine crossfade hides the click but not the shape truncation; visible as
the phase dot teleporting.

**Proposed fix:** Per-ratio "beats per alignment" table — for ratio p/q in lowest
terms, reset every q clock beats:
`{16, 8, 6, 4, 3, 2, 3, 1, 2, 1, 1, 1, 1, 1, 1}`
(i.e. /1.5 → every 3 beats, x1.5 → every 2 beats, everything else unchanged).

**Before fixing:** Audition x1.5 in Rack to confirm the every-beat retrig isn't
a desirable groove. It is mathematically inconsistent with every other ratio.

---

### [ ] 3. Phase dot desyncs from trace in free-running mode when swing is set

**Where:** `process()` stores raw swing at `src/AnalogLFO.cpp:768`;
buffer generation gates on clocked at `:795`; `drawPhaseDot()` remaps at
`:1060-1066` with no clocked gate.

**Problem:** `displaySwingFraction` is stored unconditionally as
`SWING_FRACTIONS[swingIndex]`, but the display buffer is generated with swing
zeroed when unclocked (`isClocked ? swingFrac : 0.5f`). `drawPhaseDot()` warps
the dot using the raw stored value. Result: select Medium/Heavy swing from the
context menu, unplug the clock → dot motion is swing-warped while trace and
audio are straight; dot position no longer corresponds to actual output voltage.

**Proposed fix (one line):** Store the effective display value instead:
`displaySwingFraction.store(isClocked ? swingFrac : 0.5f, ...)`.

---

## Medium priority — robustness & release blockers

### [ ] 4. Patch-load crash on malformed JSON

**Where:** `dataFromJson()` — `src/AnalogLFO.cpp:640-641`

**Problem:** `std::stoull(json_string_value(s0J), nullptr, 16)` after checking
the keys exist but not that they're strings. Hand-edited/corrupted patch →
`json_string_value` returns NULL (UB constructing `std::string`) or `stoull`
throws uncaught → Rack crashes on patch load.

**Proposed fix:** Guard with `json_is_string()`, wrap parse in try/catch (or
`strtoull` which doesn't throw), fall back to keeping the existing seed.

### [ ] 5. No LICENSE file

`plugin.json` declares GPL-3.0-or-later; the Makefile has
`DISTRIBUTABLES += $(wildcard LICENSE*)` but no LICENSE file exists.
Required for VCV Library submission and by GPL itself. Add the GPL-3.0 text
as `LICENSE` at repo root.

### [ ] 6. plugin.json empty URLs

`authorUrl`, `pluginUrl`, `sourceUrl` are all `""`. VCV Library submission
needs at least `sourceUrl`. Fill in before submitting.

### [ ] 7. Trial/proprietary fonts committed to repo

`BCBarellTEST-Regular.otf` (the "TEST" suffix = trial font — trial licenses
almost always prohibit redistribution) and `FoundationLogo.ttf` are tracked at
repo root. They're mockup-only (not shipped in `res/`), but the repo becomes a
public redistribution the moment it's published for the Library submission.
Move them out of the repo / gitignore + purge from history before going public.
`res/fonts/JetBrainsMonoNL-Regular.ttf` is OFL — fine to ship.

---

## Newly observed — 2026-06-15 (post-Phase-23 audition)

### [ ] 13. Phase Offset CV hover tooltips appear swapped with FM

**Where:** tooltip/label wiring for the FM knob + FM input jack vs. the Phase
Offset CV knob + Phase Offset CV input (`src/AnalogLFO.cpp` — param/port
`configParam`/`configInput` tooltip strings + their widget positions).

**Problem (observed in Rack):** mousing over the **FM knob** and **FM input
jack** shows the hover labels for **"Phase Offset CV"** and **"Phase Offset CV
Input"**. The two look swapped.

**Open question (determine before fixing):** which side is authoritative — the
**panel graphic labels** or the **hover/tooltip strings**? Confirm by cross-
checking the `configParam`/`configInput` tooltip text against each widget's
panel position (and the actual modulation each param/port drives) so the fix
corrects the wrong one, not the right one.

**Disposition:** fix later (not in Phase 23). Likely a `configParam`/`configInput`
ordering or copy/paste mismatch; verify against the SVG panel labels.

---

## Low priority — cleanups & polish

### [ ] 8. Dead code from uncommitted 20-03 tweaks

- `drawZeroCrossing()` (`src/AnalogLFO.cpp:1000`) — no longer called after the
  working-tree diff removed its call site.
- `scanlineImage` member (`src/AnalogLFO.cpp:861`) — unused since the
  image-pattern scanline approach was replaced with direct rect fills.

Delete both before committing 20-03.

### [ ] 9. `isStill` dim condition is unreachable

`src/AnalogLFO.cpp:1428` checks `rate <= 0.001f`, but the Rate param minimum is
0.01 — the "dim display when stopped" behavior can never trigger. Either lower
the param minimum to 0, compare against the effective (clocked) frequency, or
delete the condition.

### [ ] 10. Ratio and BPM pills skip their fade-out on clock disconnect

`displayRatioIndex` goes to -1 immediately on disconnect; `drawBpmStack()`
(`src/AnalogLFO.cpp:1202`) and the ratio label (`:1351`) early-return on it, so
both pop off instantly while the SYNC badge fades over 200ms. Cache the last
ratio/period for the fade duration to make disconnect symmetric.

### [ ] 11. Display animations are frame-rate-dependent

`WaveformDisplay::step()` (`src/AnalogLFO.cpp:868-879`) advances breathe/blink/
scanline phases by fixed `1/60` ticks. On a 144Hz monitor the 5s breathing glow
becomes ~2s and the 2Hz SYNC blink runs ~5Hz. Use
`APP->window->getLastFrameDuration()` for wall-clock-stable rates.

### [ ] 12. Display buffer regeneration runs on the audio thread

`updateDisplayBuffer()` (`src/AnalogLFO.cpp:389`, called from `process()` at
`:794-796`) computes 256 × `computeMorphedWave` (~1,500 transcendentals,
~15-60µs) inside a single sample callback. Absorbed by engine block buffering
and has shipped fine through four milestones, but it's the biggest real-time
margin eater as instance counts grow. The buffer depends only on
morph/character/swing (all already bridged via atomics) — could move wholesale
to the GUI thread in `WaveformDisplay::step()`. Not urgent; first place to look
if anyone reports crackle.

---

## Suggested order of attack

1. #3 and #4 — one-liners, zero risk
2. #1 — consecutive-outlier counter, small and self-contained
3. #2 — needs a listening test first, then the alignment table
4. #5-#7 — batch as a "release prep" pass before VCV Library submission
5. #8-#9 — fold into the 20-03 commit
6. #10-#12 — opportunistic, whenever touching display code next
