---
phase: 27-notion-manual
reviewed: 2026-07-10T00:00:00Z
depth: standard
files_reviewed: 3
files_reviewed_list:
  - src/AnalogLFO.cpp
  - res/AnalogLFO.svg
  - panel-overlay.html
findings:
  critical: 0
  warning: 1
  info: 2
  total: 3
status: issues_found
---

# Phase 27: Code Review Report

**Reviewed:** 2026-07-10T00:00:00Z
**Depth:** standard
**Files Reviewed:** 3
**Status:** issues_found

## Summary

Scope was limited to the three phase-27 changes: (1) the CV attenuator `configParam`
default flip from `1.f` to `0.f` in `src/AnalogLFO.cpp`, (2) the FM/PHASE trim-label
glyph x-coordinate swap in `res/AnalogLFO.svg`, and (3) the new `panel-overlay.html`
callout-baking tool.

Verdict on the two in-place edits: **both are correct.** The `configParam` default
change is safe and cannot cause data loss or a crash. The SVG label swap is coherent —
the swapped glyphs decode to the right text and now sit over the correct CV columns.

The one substantive finding is a robustness bug in the new `panel-overlay.html`: the
render/completion logic hangs off `img.onload` without an `img.complete` guard, so a
cached or already-loaded background image can leave the overlay blank and never set the
`window.__done` completion signal the baking workflow relies on.

## Narrative Findings (AI reviewer)

### Verified-correct changes (no finding)

**`configParam` attenuator defaults `1.f` -> `0.f` (AnalogLFO.cpp:203-205, 212, 214).**
All five CV attenuators (MORPH/CHARACTER/DRIFT lines 203-205, PHASE_OFFSET line 212, FM
line 214) now default to `0.f`. Traced through `process()`:
- MORPH/CHARACTER/DRIFT (L286/293/300): `clamp(k + a*cv/5, 0, 1)` with `a=0` -> CV term
  vanishes, no NaN/overflow path.
- PHASE_OFFSET (L303-311): `a` is only read inside the `isConnected()` guard; `a=0` -> `cv=0`.
- FM (L315): `in.fmAtten = 0` -> core applies zero FM depth.
None can crash or produce out-of-range values. **No patch-compatibility data loss:** Rack
serializes live param values into the patch, so previously-saved patches restore their
stored attenuator values on load; the new default only affects freshly-placed modules and
"Initialize". The change is correct and safe.

**FM/PHASE trim-label glyph swap (AnalogLFO.svg:146, 155).** Confirmed coherent by
decoding both glyph paths:
- Line 155 (`translate(39.3389,103.9) scale(0.0015)`) = "FM"; glyph center ~40.28mm,
  aligning to the x=40.29 FM_ATTEN trimpot / FM input column.
- Line 146 (`translate(48.7113,103.9) scale(0.0016)`) = "PHASE"; glyph center ~51.16mm,
  aligning to the x=51.15 PHASE_OFFSET_ATTEN / PHASE_OFFSET_CV column.
These match the `addParam`/`addInput` x-coordinates (AnalogLFO.cpp:1147-1150, 1159-1162)
and the `CTRL` map in `panel-overlay.html` (indices 10=FM@40.29, 11=PHASE@51.15). Each
glyph retained its own `scale`, so only the x offsets were exchanged. Correct.

## Warnings

### WR-01: `img.onload` render logic has no `img.complete` fallback — cached image can leave overlay blank and hang the baking workflow

**File:** `panel-overlay.html:50-63`
**Issue:** All SVG leader lines and badge DIVs are appended, and the completion flag
`window.__done=true` is set, exclusively inside `document.getElementById('panel').onload`.
The `<img id="panel" src="docs/img/panel-raw.png">` element is parsed before the script
runs (L27 vs L30), so if the image is already cached / completes before the handler is
attached, the `load` event will not re-fire — the overlay renders nothing and `window.__done`
is never set. Since the header comment describes an HTTP-served, repeatedly-screenshotted
re-bake workflow, a warm cache is the normal case on the second run. A missing/404
`panel-raw.png` produces the same silent-blank outcome with no diagnostic.
**Fix:** Guard for the already-loaded case and factor the body into a function:
```javascript
const panelImg = document.getElementById('panel');
function render(){ /* existing onload body */ }
if (panelImg.complete && panelImg.naturalWidth) render();
else { panelImg.onload = render; panelImg.onerror = () => console.error('panel-raw.png failed to load'); }
```

## Info

### IN-01: Attenuator default-0 is a user-visible behavior change the manual must document

**File:** `src/AnalogLFO.cpp:203-205, 212, 214`
**Issue:** These are unipolar attenuators (range `[0.f, 1.f]`), so the new default of `0.f`
is the *minimum* — a freshly-placed module ignores all patched CV until the user manually
opens each attenuator. This is a legitimate, common design choice, but it is a silent
behavior change relative to the prior fully-open default and is exactly the kind of thing a
new user will read the manual to understand. Since this is the manual phase (27-notion-manual),
verify `docs/panel.md` / the Notion manual explicitly states that CV attenuators start at 0
and must be turned up for CV to take effect.
**Fix:** Add a note in the CV/attenuator section of the manual describing the default-closed
behavior; no code change required.

### IN-02: Overlay transform constants are unlabeled magic numbers

**File:** `panel-overlay.html:31, 41-46`
**Issue:** The calibration constants (`PPM=11.8491, LX=-3.25, TY=-7.97, MX, MY`) and the
per-group layout offsets (`120/142/268`, `-96/-74/-66`, `1685/1663`, `1600/1578/+51`,
`gx=c-66`) are hardcoded literals. `LX`/`TY` in particular are empirically fitted to a
specific screenshot and cannot be verified in review; if `panel-raw.png` is re-captured at a
different scale or crop, every callout silently mis-registers with no assertion or check.
For a one-off baking tool this is acceptable, but the derivation is fragile.
**Fix:** Keep the header comment's derivation note (good), and consider adding a visible
registration check (e.g., draw a small crosshair at `cc(83.74,119.5)` and `cc(5,19)`) so a
mis-scaled screenshot is caught by eye during the bake instead of shipping a skewed overlay.

---

_Reviewed: 2026-07-10T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
