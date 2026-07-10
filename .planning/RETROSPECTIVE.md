# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — Analog Series LFO

**Shipped:** 2026-03-07
**Phases:** 6 | **Plans:** 12 | **Total execution:** 58 min

### What Was Built
- Complete three-knob analog engine (morph, character, drift) with CV control
- Four-shape parametric morph oscillator (Sine-Tri-Saw-Square)
- Per-shape analog character modeling targeting real synths (Minimoog, Roland, Moog/Prophet)
- Four-layer Ornstein-Uhlenbeck drift engine with per-module RNG
- Real-time waveform display with lock-free double buffer and NanoVG rendering
- 12HP branded panel with designer-ready SVG

### What Worked
- Strict dependency chain (scaffold → engine → display → character → drift) kept each phase focused
- Visual verification plans (even-numbered plans) caught real issues: display occluding knobs, triangle phase inversion, square shape misalignment
- Research phases before planning produced well-targeted DSP implementations
- Falling saw convention decision (Phase 2) elegantly solved the morph crossfade amplitude dip
- Characterize-then-morph ordering (Phase 4) gave coherent transitions without special cases

### What Was Inefficient
- Character deformation amplitudes from research (2-3%) were too subtle at LFO rates — had to increase to 8-50% for perceptibility. Research should account for sub-audio vs audio-rate perception differences.
- Drift dot instability parameters needed two rounds of tuning (initial was too subtle, then 5x amplification in Phase 6). Could have been more aggressive in initial implementation.
- Bottom row layout went through three iterations (Phase 2, 5, 6) as component count changed. Earlier component inventory would have avoided rework.
- Phase 4 (Pitch Controls with octave/semitone) was planned then removed — not meaningful for sub-audio LFO. Better domain analysis during requirements would have caught this.

### Patterns Established
- Lock-free atomic transfers for audio-to-GUI data (displayBuffers, displayPhase, displayDrift)
- Even-numbered plans as visual verification in VCV Rack (human-in-the-loop)
- Progressive x^2 knob curves for subtle-to-aggressive response
- Two-row bottom layout convention (trimpots above jacks)
- Per-module RNG seeding for unique behavior per instance

### Key Lessons
1. Sub-audio LFO rates need larger deformation amounts than audio-rate research suggests — users see the waveform, not just hear it
2. Visual verification catches layout and interaction issues that code review cannot — worth the extra plan
3. Milestone audit before completion catches stale docs and subtle issues (displayDrift CV gap) that accumulate during fast execution
4. Panel layout should be designed for final component count, not incrementally expanded

### Cost Observations
- Model mix: quality profile throughout
- Sessions: ~6 planning + execution sessions
- Notable: 58 min total execution across 12 plans (4.8 min avg) — very efficient for a complete DSP module

---

## Milestone: v1.1 — Clock Sync

**Shipped:** 2026-03-13
**Phases:** 4 | **Plans:** 6 | **Timeline:** 6 days

### What Was Built
- Three-state clock tracker (FREE/ACQUIRING/LOCKED) with EMA smoothing and outlier rejection
- 15 discrete musical ratios (/16 to x16) with dual-mode Rate knob
- Division-aware phase reset with 3ms cosine crossfade for click-free beat alignment
- Drift authority scaling and frequency slew for smooth mode transitions
- NanoVG text overlays (SYNC badge, ratio label, BPM, Hz) with 200ms fade animations
- Panel SVG CLK jack with lavender label

### What Worked
- Layered phase approach: clock detection → ratio mapping → phase reset → display kept each phase tightly scoped
- Human verification plans (07-02, 10-02) caught zero issues — implementations were correct on first pass
- Atomic display bridge pattern from v1.0 (displayPhase, displayDrift) extended cleanly for three new atomics (displayClockState, displayRatioIndex, displaySmoothedPeriod)
- Research before Phase 7 produced the right algorithm (EMA vs PLL vs Kalman decision) — no rework
- Phase 9 crossfade approach (capture lastOutputVoltage before reset) avoided coupling processClockInput() to morph/character state

### What Was Inefficient
- Phase 8 and 9 ROADMAP.md checkboxes were not updated after completion — caught during audit. Roadmap status tracking should be automated or checked at plan completion.
- SUMMARY.md files lack `one_liner` field — gsd-tools summary-extract returns null. Either populate one_liner during plan completion or skip extraction.

### Patterns Established
- Relaxed atomics for all audio-to-GUI bridges (consistent with v1.0, now 5 atomics total)
- Division-aware clock counting pattern for /N ratios using integer beat counter
- SVG text-as-paths for nanosvg compatibility (CLK label as three separate path elements)
- NanoVG two-pass glow text rendering (blur pass + sharp pass)
- Fade animation state machine for smooth text overlay transitions

### Key Lessons
1. First-pass implementations that follow research closely tend to pass verification without changes — invest in research quality
2. The atomic bridge pattern scales well — adding new display data requires only declaring the atomic and wiring store/load
3. Clock-synced DSP is simpler than expected when built on a clean state machine foundation — the layered approach (detect → divide → reset → display) kept complexity manageable

### Cost Observations
- Model mix: quality profile (opus planning, sonnet execution/verification)
- Notable: All 6 plans executed cleanly; zero rework needed post-verification

---

## Milestone: v1.2 — Deep Analog

**Shipped:** 2026-03-17
**Phases:** 6 (1 skipped) | **Plans:** 8 | **Timeline:** 5 days

### What Was Built
- Pill-backed HUD overlays with feathered nvgBoxGradient backgrounds for waveform readability
- Independent RESET trigger with bidirectional 1ms blanking reusing existing crossfade
- Phase Offset knob/CV applied at readout (not accumulator) preserving all timing behavior
- Exponential FM input with dual-authority scaling (0.5f clocked, 1.0f free)
- Expanded analog imperfections: phase jitter, DC offset wander, pitch slew, component spread
- Waveform bleed via wrapping ring topology with proximity-weighted neighbor crosstalk
- MPC-style swing timing with 6 named presets via right-click context menu

### What Worked
- Research-then-plan pattern continued to produce first-pass implementations that work correctly
- Offset-at-readout design decision (Phase 12) avoided accumulator coupling — no side effects on clock/drift
- Component spread serialization as hex strings (Phase 14) avoided uint64_t JSON sign issues caught in research
- Wrapping ring topology (Phase 15) made waveform bleed clean and extensible via modular arithmetic
- Swing as deltaPhase multiplier (Phase 16) was commutative with drift/jitter — no ordering issues
- Phase 17 skip decision was correct — panel density analysis during context gathering prevented wasted effort

### What Was Inefficient
- SUMMARY.md files still lack `one_liner` field — gsd-tools summary-extract returns null (same issue as v1.1)
- Phase 14 ROADMAP.md plan checkboxes not marked as [x] despite being complete — carried forward from v1.1 audit finding
- v1.2 controls placed at temporary panel positions that will need rework when modulation routing is designed

### Patterns Established
- Right-click context menu for controls that don't fit on panel (swing presets)
- Phase skip with documented rationale when scope analysis reveals wrong approach
- DC offset applied after crossfade capture to prevent clicks on phase reset
- Component spread with deterministic RNG seed for per-instance variation with serialization
- Normalization divisor pattern (1 + intensity) for guaranteed amplitude safety

### Key Lessons
1. Phase skip is a valid outcome when context gathering reveals fundamental approach problems — better than forcing a bad design
2. Temporary panel positions create tech debt that compounds — each new feature adds another "temporary" control
3. The three-knob engine scales well — 6 phases of new features layered cleanly onto the v1.0/v1.1 foundation
4. Modulation routing (Surge-style MOD inputs) is the right abstraction for panel density — per-parameter CV architecture doesn't scale

### Cost Observations
- Model mix: quality profile throughout
- Sessions: ~4 planning + execution sessions
- Notable: 8 plans in ~97 min — consistent with v1.0 velocity

---

## Milestone: v1.3 — Forge Noir

**Shipped:** 2026-06-13
**Phases:** 5 (incl. 20.1 inserted) | **Plans:** 14 | **Timeline:** 2026-03-28 → 2026-06-13

### What Was Built
- Five-shape PWM morph sweep (Sine→Tri→Saw→Square→Pulse) with tanh edge softening, 50%→5% duty, and 5-shape bleed ring wrap
- Forge Noir custom-component panel: 11 nanosvg skins (3 knob sizes, scalloped trimpots, ember-ring jacks, hex bolts) as 7 widget structs
- Near-black panel artwork with 110+ path letterforms, forge emblem, rune glyph, gradient placement
- Three-column CRT display: margin pills, center ember waveform, corner brackets, scrolling scanlines, breathing border glow
- 18HP "fresh" panel redesign (inserted Phase 20.1) with widget-owned knobs and grouped clock section
- Per-edge animated SYNC badge flash via lock-free atomic counter + exponential decay

### What Worked
- SUMMARY.md `one_liner` fields were finally populated — `summary-extract` returned clean data, closing the recurring v1.1/v1.2 gap and letting milestone close auto-generate accomplishments
- The v1.0 atomic audio-to-GUI bridge pattern extended once more for Phase 21's `displayClockEdge` with zero new races on the audio thread
- The 18HP redesign as an explicitly inserted phase (20.1) cleanly contained a mid-milestone art change without disrupting numbering
- Milestone audit caught accumulated verification/doc debt (stale VERIFICATION statuses, WAVE-05 contradiction, dead code) and it was resolved inline before archive rather than shipped silently

### What Was Inefficient
- Panel size churned 12HP → 14HP → 18HP across the milestone; Phase 19 built and verified a 14HP panel that Phase 20.1 then replaced, wasting the 14HP visual-UAT effort. Earlier commitment to final HP would have avoided a full panel rebuild — the same "design for final size" lesson from v1.0, recurring.
- Several phase VERIFICATION.md statuses were left at `human_needed` after the human gate was actually performed — bookkeeping lag that only the audit caught (recurring roadmap/status-tracking gap from v1.1/v1.2).
- WAVE-05 was written as "preserve backward compat" but the implementation intentionally broke it (D-02); the requirement text and the code diverged until the audit forced a rewrite. Requirement wording should track scope decisions as they happen.

### Patterns Established
- Inserted decimal phase (20.1) for a mid-milestone redesign that supersedes an earlier phase's artifact, with the superseded phase's UAT explicitly marked superseded rather than failed
- Widget-owned knob rendering (strip metal bodies from SVG, keep only recessed-socket shadows) as the canonical convention
- Single production panel file auto-deriving HP from viewBox (no plugin.json width) to prevent art/code drift
- Color/glow-driven animation (not alpha) for badge flash, keeping the rest-state render byte-identical via defaulted params

### Key Lessons
1. "Design for final component count" is now a thrice-confirmed lesson (v1.0 bottom row, v1.2 temp positions, v1.3 HP churn) — lock panel dimensions before building custom components against them
2. The milestone audit is the safety net that catches verification/doc debt accumulated during fast (and slow, gap-filled) execution — it earned its keep again
3. When a scope decision breaks a stated requirement, rewrite the requirement at decision time, not at milestone close
4. Populating SUMMARY one_liners pays off immediately at milestone close — the long-standing gap is worth closing per-plan

### Cost Observations
- Model mix: quality profile throughout (Opus planning, Sonnet execution/verification)
- Notable: calendar timeline was long (~2.5 months) but sparse — real work was a handful of focused sessions; the panel redesign drove most of the added scope

---

## Milestone: v1.4 — Tempered

**Shipped:** 2026-07-10
**Phases:** 7 | **Plans:** 28 | **Tasks:** 54 | **Timeline:** 2026-06-14 → 2026-07-10

### What Was Built
- Rack-independent header-only DSP core (`src/dsp/*.hpp`) proven bit-exact vs the shipped inline DSP, consumed by a thinned plugin shell
- `make test` doctest harness (never links libRack) + headless `BlockDriver` invariant suite + bit-exact golden regression + 3-OS GitHub Actions CI
- Four functional bug fixes (clock >3× lockout, x1.5/÷1.5 cadence, free-run swing phase-dot, corrupt-patch crash guard), each pinned by a red→green regression
- Five display/thread cleanups: 256× fill moved off the audio thread (seqlock snapshot), frame-rate-independent animation, pill fade symmetry, dead-code removal
- Release IP hardening: trial fonts purged from full git history (verified clean via fresh mirror), GPL-3.0 LICENSE + NOTICES + OFL, SVG outlines re-exported from confirmed-OFL fonts — all while PRIVATE
- VCV compliance: validated manifest, verified `.vcvplugin`; GitHub-Markdown user manual under `docs/`; public repo flip + VCV Library submission issue #929

### What Worked
- **Test harness first (Phase 22) as a load-bearing safety net** — every later DSP refactor and bug fix rode on green unit tests + a golden bit-exact replay, so the full core extraction and the two-cell cadence swap landed with provable no-regression
- **The PRIVATE-first purge → verify → flip ordering** held: verifying the trial-font purge on a fresh `--mirror` clone *after* the tag/push but *before* the public flip meant no branch or tag could carry a purged blob at the moment of exposure
- **Human go/no-go gates for every irreversible outward-facing action** (public flip, VCV submission) kept the operator in control; the flip and the submission each ran only after an explicit CLEAN verdict + approval
- Populated SUMMARY one_liners again let milestone close auto-generate accomplishments (gap stayed closed from v1.3)

### What Was Inefficient
- **Requirements traceability drift, again:** 8 requirements (TEST-03, BUG-01/02, PKG-01/04, IP-01/02/03) stayed `Pending` in REQUIREMENTS.md despite being delivered and phase-verified — the same status-bookkeeping lag flagged in v1.1/v1.2/v1.3. Caught at milestone close and reconciled inline against phase summaries + on-disk evidence.
- Two manual in-Rack UAT scenarios (BUG-03/BUG-04) were never executed during the milestone; deferred at close with automated regression coverage as the standing safety net.
- The internal milestone label (v1.4) vs the published release version (v2.0.0, Rack-major) is a mismatch that required deliberate care to avoid mistagging the release.

### Patterns Established
- **"Verify clean at the moment of exposure"**: a fresh independent `--mirror` re-clone + `rev-list --all --objects` grep, plus a belt-and-braces purged-blob-OID absence check, as the hard gate on an irreversible public flip
- **Submission pins an immutable 40-char commit hash** (never a branch/tag), and **one permanent issue thread** carries all future version updates
- **Test-harness-before-refactor** as an explicit, enforced milestone sequencing constraint
- **AskUserQuestion human go/no-go** as the standard gate for irreversible, outward-facing actions

### Key Lessons
1. Requirements-status drift is now a **four-milestone recurring** catch — the milestone-close reconciliation is the reliable net, but updating requirement status at `phase.complete` time would stop it accumulating in the first place
2. For irreversible outward-facing actions (history purge, public flip, external submission), gate on a **fresh independent re-verification + explicit human confirmation**; belt-and-braces (path grep *and* blob-OID absence) beats a single check
3. Reconcile the **internal milestone label and the published release version** explicitly (v1.4 → tag v2.0.0) so the release is never mistagged
4. A green test harness built *first* converts risky refactors (full DSP extraction, cadence changes) into provable no-regression changes — the safety net pays for itself immediately

### Cost Observations
- Model mix: Opus executor, Sonnet verifier (quality profile)
- Notable: the publish/submit phase ran inline sequential (no subagents) so the operator stayed in the loop for each irreversible step — the right trade of parallelism for control

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Timeline | Phases | Plans | Key Change |
|-----------|----------|--------|-------|------------|
| v1.0 | 10 days | 6 | 12 | Established visual verification pattern |
| v1.1 | 6 days | 4 | 6 | Layered DSP approach; zero rework post-verification |
| v1.2 | 5 days | 6 (+1 skip) | 8 | Phase skip pattern; right-click menu for panel overflow |
| v1.3 | ~2.5 mo (sparse) | 5 (+1 insert) | 14 | Inserted redesign phase; audit-driven debt cleanup before archive |
| v1.4 | ~26 days | 7 | 28 | Release-hardening milestone; test-harness-first; PRIVATE purge gated the irreversible public flip |

### Cumulative Stats

| Milestone | LOC (total) | LOC Added | Requirements |
|-----------|-------------|-----------|-------------|
| v1.0 | 552 | 552 | 23/23 |
| v1.1 | 890 | 338 | 18/18 |
| v1.2 | 1,374 | 484 | 15/17 (2 deferred) |
| v1.3 | 1,641 | 267 | 24/24 |
| v1.4 | 2,435 src + 1,349 tests | +794 src (DSP core extraction) + full test harness | 28/28 (2 manual UAT deferred) |

### Top Lessons (Verified Across Milestones)

1. Visual verification plans are essential for UI-heavy modules — catches issues code review misses (v1.0 confirmed, v1.1 reconfirmed)
2. Research before planning produces first-pass implementations that pass verification without rework (v1.0 character amplitudes exception, v1.1 and v1.2 fully validated)
3. Atomic bridge pattern for audio-to-GUI data scales cleanly across milestones — declare, store, load (v1.3: 6th atomic added cleanly)
4. Phase skip is a valid and valuable outcome when context gathering reveals fundamental approach problems (v1.2 Phase 17)
5. Design for final panel dimensions before building components — incremental HP/layout expansion causes rework every milestone (v1.0, v1.2, v1.3)
6. The milestone audit reliably surfaces verification/doc debt before it ships — run it every milestone (v1.0 displayDrift gap, v1.3 stale statuses + WAVE-05)
7. SUMMARY.md one_liner fields: gap finally closed in v1.3 — keep populating per-plan (v1.4 reconfirmed)
8. Requirements-status bookkeeping drifts every milestone (v1.1–v1.4) — reconcile at phase.complete, not just at milestone close
9. Irreversible outward-facing actions need a fresh independent re-verification + explicit human go/no-go before firing (v1.4: purge re-verify → public flip → VCV submission)
10. Build the test harness before the refactor — it turns risky changes into provable no-regressions (v1.4 DSP extraction + cadence swap)
