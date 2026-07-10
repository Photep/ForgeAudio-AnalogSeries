# Milestones

## v1.4 Tempered (Shipped: 2026-07-10)

**Phases completed:** 7 phases, 28 plans, 54 tasks

**Key accomplishments:**

- Rack-free `make test` target running a vendored doctest 2.4.11 harness across two translation units, fully additive — the plugin build is untouched and links no libRack.
- The four leaf DSP headers (RackCompat, Waveshape, RatioTable, Swing) now live under `src/dsp/` as header-only `namespace forge` code with zero `rack/` includes, bit-identical to the shipped inline DSP, with the OU-layer-0 bleed coupling lifted to an explicit `bleedLfo` parameter (D-05) and exercised by Rack-free doctest unit cases.
- TEST-02 full core extraction effectively LANDED HERE (Phase 22), not Phase 24.
- The headless `tests/BlockDriver.hpp` drives the extracted `forge::LfoCore` over sample blocks, the full TEST-04 invariant suite (±5V bounds, ±1% free-run frequency, ≤0.5V reset continuity, bit-exact fixed-seed determinism) passes at 44.1/48/96 kHz, the permanent D-08 golden replay pins the core bit-exact to the frozen `.f32` baselines on the canonical OS, and a Rack-free 3-OS GitHub Actions matrix runs `make test` on every push — completing the v1.4 safety net.
- Consecutive-outlier counter in ClockTracker.hpp that re-acquires after a sustained >3x speedup (and in the fast-clock slowdown band), pinned by a demonstrated RED->GREEN BUG-01 regression.
- Non-throwing `forge::parseSeedHex` (strtoull + ERANGE/endptr) wired into `dataFromJson` closes the malformed-seed patch-load crash (BUG-04, red→green), and the phase-dot display swing is gated to its effective value so the dot tracks the trace in free-run+swing (BUG-03).
- Extended `test_dsp_units.cpp` to cover the full waveshape output-range grid, the `shouldReset` reset cadence at all 13 un-gated ratios, and swing math (free-run gate + clocked Straight/Heavy warp) — `make test` 38 → 42 cases, all green.
- BEATS_PER_ALIGN[15] table swap fixes x1.5/÷1.5 mid-cycle truncation per the logged adopt-table audition decision, pinned by a deterministic red→green reset-cadence regression.
- Two pure, Rack-free header helpers — `forge::fillDisplayBuffer` (preview loop with `bleedLfo` as a parameter) and `forge::clampFrameDt`/`flashDecay` (isfinite-guarded dt clamp + continuous decay) — each pinned by a headless doctest TU, with `make`/`make test` staying green.
- Moved the heavy 256x `morphedWave` preview fill off the real-time audio thread: `process()` now publishes a tear-free seqlock snapshot {morph, character, effective swingFrac, bleedLfo} captured at the trigger instant, and `WaveformDisplay::step()` consumes it (acquire/retry) and runs the fill GUI-side via the pure `forge::fillDisplayBuffer` — preview byte-identical, audio callback wait-free.
- Landed the three GUI-widget cleanups on `WaveformDisplay` now that the fill is off the audio thread (24-02): every `step()` animation advances by one clamped wall-clock dt (`forge::clampFrameDt(getLastFrameDuration())`) with the ANIM-02 badge decay preserved via `forge::flashDecay`, the ratio/BPM pills fade out symmetrically with the SYNC badge through a widget-side `cachedRatioIdx`/`cachedPeriod` cache, and the dead `drawZeroCrossing`/`scanlineImage` plus the unreachable `isStill` branch are gone — all GUI-thread-only, no atomic/DSP change.
- The deterministically-untestable parts of Phase 24 — animation feel/frame-rate independence (CLEAN-04), ratio/BPM pill fade symmetry (CLEAN-03), audio-thread relief (CLEAN-05), and unchanged display after dead-code removal (CLEAN-01/02) — were human-verified in VCV Rack and signed off APPROVED, with Assumption A1 (getLastFrameDuration returns the inter-frame interval) closed confirmed-by-behavior and the full outcome logged to STATE.md ahead of /gsd:verify-work.
- The working tree is now legally clean and complete for a public GPL-3.0 release: trial fonts gone + gitignored, verbatim GPL-3.0 LICENSE + NOTICES + OFL.txt present and verified shipping in the dist artifact, with tests still 47/47 green.
- The shipped panel SVGs embed no font program (automated census, 0 font/glyph nodes across all 10 files), but the operator could NOT positively confirm the FORGE/AUDIO + "ANALOG LFO" wordmark outlines derive from OFL fonts — PROVENANCE DECISION: needs-regeneration. This BLOCKS the irreversible 25-03 history purge until the wordmark is re-exported from a confirmed-OFL font.
- Both trial-font blobs are permanently purged from ALL git history on the still-private remote (all branches + tag v1.3), and an independent clean-room re-clone verifies EMPTY — the IP-02 hard gate Phase 28's public flip depends on. Push-first preserved every Phase 22-25 commit; the repo stayed private throughout.
- All 18 baked-text outlines in res/AnalogLFO.svg were re-exported from confirmed-OFL fonts (Chakra Petch, whole panel) via a throwaway fontTools SVGPathPen pipeline, replacing geometry that almost certainly derived from the trial FoundationLogo. The panel stays path-only (0 font programs), the forge-rune and all non-text art are byte-preserved, and the operator visually accepted the result — PROVENANCE DECISION: confirmed-OFL. IP-03 is closed; 25-03 is unblocked.
- plugin.json is field-by-field submission-ready — all three manifest URLs populated to the operator-owned GitHub repo, minRackVersion lowered to 2.0.0, and every PKG-03 field rule (version, VERSION derivation, tag whitelist, slug charset/immutability, no trademarked strings) validated; URL HTTP-200 reachability is the one item deferred to the Phase 28 public flip.
- Portable drift-off + spread-off golden fixtures with a Rack-free generator, making the cross-platform CI golden leg green on Linux/Windows while keeping the non-portable drift-on bit-exact replay macOS-gated.
- docs/ GitHub-Markdown manual scaffold with a hub and four verbatim-from-source reference sections (engine concept, I/O with CV ranges, Swing menu, clock/sync FSM + 15-ratio table).
- Task 1 — install + license/credits
- Added `manualUrl` (+ `changelogUrl`) to a valid `plugin.json` (version untouched at 2.0.0) and reconciled ROADMAP §Phase 27 and REQUIREMENTS DOC-01/02/03 from the Notion plan to the GitHub-Markdown `docs/` pivot with patch-examples dropped.
- Produced the D-08 annotated-panel deliverable — a real VCV Rack screenshot of the shipped ForgeAnalogLFO with 19 numbered ember-ring callouts baked in and keyed 1:1 to the panel.md legend — and, along the way, fixed a discovered panel label bug, changed the CV attenuator defaults, and rewrote the manual in an end-user voice.

---

## v1.3 Forge Noir (Shipped: 2026-06-13)

**Phases completed:** 5 phases (18, 19, 20, 20.1 inserted, 21), 14 plans, 20 tasks

**Key accomplishments:**

- Five-shape morph sweep with PWM — variable-width pulse as the 5th shape (Sine→Tri→Saw→Square→Pulse), tanh edge softening, 50%→5% duty cycle, and 5-shape bleed ring wrap (Phase 18, WAVE-01..05)
- Forge Noir custom components — 11 nanosvg component skins (3 knob sizes, scalloped trimpots, ember-ringed output jack, hex bolts) wired as 7 custom widget structs (Phase 19, PANEL-01..07)
- Forge Noir panel artwork — near-black panel with 110+ path letterforms, forge emblem, rune glyph, and gradient component placement (Phase 19)
- Three-column CRT display — pills in dedicated left/right margins, ember-glow waveform constrained to center, corner brackets, scrolling scanlines, and breathing border glow (Phase 20, DISP-01..05)
- 18HP fresh panel redesign — promoted to production `res/AnalogLFO.svg` with widget-owned knobs, single-row equal secondary knobs, grouped clock section, all anchors aligned at 100%/200% (Phase 20.1, PANEL-08..12)
- Animated SYNC badge — per-edge white-hot flash while LOCKED via lock-free atomic counter and 0.92×/frame exponential decay envelope (Phase 21, ANIM-01..02)

**Stats:**

- 1,641 lines of C++ (`AnalogLFO.cpp`), 106 commits, 96 files changed, +16,160 / −2,455
- Timeline: 2026-03-28 → 2026-06-13
- Requirements: 24/24 complete (WAVE 5, PANEL 12, DISP 5, ANIM 2)
- Milestone audit: passed (tech-debt resolved inline — WAVE-05 rescoped per D-02, verification bookkeeping flipped, dead code removed)

**Pre-close cleanup:** 3 stale todos resolved (PWM→Phase 18, display-pill separation→Phase 20, Surge-routing→out of scope); Phase 18 PWM UAT owner-approved in Rack; Phase 19 14HP panel UAT superseded by Phase 20.1 18HP redesign.

---

## v1.2 Deep Analog (Shipped: 2026-03-17)

**Phases completed:** 6 phases (1 skipped), 8 plans

**Key accomplishments:**

- Pill-backed HUD text overlays with feathered backgrounds for waveform readability (DISP-01, DISP-02)
- Independent RESET trigger with bidirectional 1ms blanking and Phase Offset knob/CV (MOD-03, MOD-04, PHASE-01, PHASE-02)
- Exponential FM input with dual-authority clocked/free mode scaling (MOD-01, MOD-02)
- Expanded analog imperfections: phase jitter, DC offset wander, pitch slew, component spread (CHAR-01 through CHAR-04)
- Waveform bleed with neighbor-shape crosstalk during morph transitions (CHAR-05)
- MPC-style swing timing with 6 named presets and context menu (PHASE-03, PHASE-04)

**Stats:**

- 1,374 lines of C++ (484 added), 44 commits
- Timeline: 5 days (2026-03-13 to 2026-03-17)
- Requirements: 15/17 complete (2 deferred: PANEL-01, PANEL-02)

### Known Gaps

- **PANEL-01**: Panel SVG update — deferred to modulation routing milestone
- **PANEL-02**: Panel control placement — deferred to modulation routing milestone
- **Phase 17** (Panel Redesign) skipped: 12HP density requires Surge-style modulation routing system

---

## v1.1 Clock Sync (Shipped: 2026-03-13)

**Phases completed:** 4 phases, 6 plans

**Key accomplishments:**

- Three-state clock tracker (FREE/ACQUIRING/LOCKED) with EMA smoothing, outlier rejection, and fast-track re-acquisition
- 15 discrete musical ratios (/16 to x16) with dual-mode Rate knob and custom tooltip
- Division-aware phase reset with anti-click 3ms cosine crossfade for beat-aligned operation
- Drift authority scaling (2% clocked vs 7.5% free) and 50ms frequency slew for smooth mode transitions
- NanoVG text overlays (SYNC badge, ratio label, BPM readout, Hz readout) with 200ms fade animations
- Panel SVG updated with CLK jack and lavender label

**Stats:**

- 890 lines of C++ (338 added), 24 files changed, 4,251 insertions
- Timeline: 6 days (2026-03-07 to 2026-03-13)
- Requirements: 18/18 complete

---

## v1.0 Analog Series LFO (Shipped: 2026-03-07)

**Phases completed:** 6 phases, 12 plans, 0 tasks

**Key accomplishments:**

- Three-knob analog engine (morph, character, drift) with CV control for all three axes
- Four-shape parametric morph: continuous Sine-Tri-Saw-Square with musically valid intermediates
- Per-shape analog character modeling: Minimoog saw, Roland square, Moog/Prophet triangle, analog sine
- Four-layer Ornstein-Uhlenbeck drift engine with per-module RNG for unique analog instability
- Real-time waveform display with lock-free double buffer, amber glow trace, and phase-tracking dot
- Polished 12HP panel with Forge Audio brand identity, two-row bottom layout, designer-ready SVG

**Stats:**

- 552 lines of C++, 72 commits, 63 files, 12,660 insertions
- Timeline: 10 days (2026-02-25 to 2026-03-07)
- Requirements: 23/23 complete

---
