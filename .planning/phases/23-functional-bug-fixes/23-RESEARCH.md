# Phase 23: Functional Bug Fixes - Research

**Researched:** 2026-06-14
**Domain:** VCV Rack plugin (C++17) — DSP bug fixes pinned by headless doctest regression tests; one human-gated in-Rack listening audition
**Confidence:** HIGH

## Summary

Phase 23 fixes four functional bugs in the Analog LFO, each pinned by a deterministic regression test. The decisive finding: **Phase 22 already extracted the entire DSP into Rack-free header-only units** (`src/dsp/ClockTracker.hpp`, `RatioTable.hpp`, `Swing.hpp`, `LfoCore.hpp`), and the extraction was done *with these exact P23 fixes in mind* — `RatioTable::shouldReset` carries a literal `// FUTURE (P23)` marker showing where the BEATS_PER_ALIGN table slots in, and `LfoCore::Telemetry` already exposes `swingFrac`/`isClocked`/`displayPhase`. Three of the four bugs (BUG-01, BUG-02, BUG-03-core-half) are **fully reachable from the existing headless harness today** — no new extraction is required. Only BUG-03's *consumer* side (the shell's `displaySwingFraction.store` + `drawPhaseDot`) and BUG-04 (`dataFromJson`) live in the Rack shell `src/AnalogLFO.cpp` and are GUI/JSON glue, not headless-testable as-is.

The BEATS_PER_ALIGN table `{16,8,6,4,3,2,3,1,2,1,1,1,1,1,1}` was verified mathematically (Fraction-reduced p/q, reset every q beats ⇒ integer cycles): it changes behavior at **exactly two indices** — idx 6 (/1.5: divisor 2→3) and idx 8 (x1.5: divisor 1→2) — and is bit-identical to the current `round(1/ratio)` math at all 13 other ratios. This makes BUG-02 a two-cell table swap.

BUG-04 is **only half-fixed in the live tree**: the `json_is_string` type guard (AnalogLFO.cpp:243) is already present, but `std::stoull` (L244-245) still throws `invalid_argument` on non-hex string content and `out_of_range` on an over-long value — both empirically confirmed uncaught (test run below). The crash path is still open; the parse must be wrapped or moved to `strtoull`.

**Primary recommendation:** Plan in three waves. Wave A (parallel): BUG-01 (ClockTracker consecutive-outlier counter) + BUG-03 + BUG-04 + their regression tests — all independent, all small. Wave B (BLOCKING human gate): the in-Rack x1.5/÷1.5 audition, decision logged to STATE.md `### Decisions` BEFORE any code. Wave C (sequential, gated by B): the BEATS_PER_ALIGN table swap + its deterministic reset-cadence regression test. Use strict TDD ordering per fix: write the test, run `make test`, **confirm it goes RED against the current header**, apply the fix, confirm GREEN.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Clock outlier recovery (BUG-01) | DSP core (`ClockTracker.hpp`) | — | Pure FSM state; already extracted & headless-driven by `test_dsp_stateful.cpp` |
| Ratio→beats alignment (BUG-02) | DSP core (`RatioTable.hpp::shouldReset`) | — | Pure table lookup; `ClockTracker` already delegates to it (single home, CR-03) |
| Swing phase warp (BUG-03 producer) | DSP core (`Swing.hpp` + `LfoCore` telemetry) | — | `LfoCore::Telemetry.swingFrac`/`.isClocked` already computed correctly |
| Phase-dot display swing (BUG-03 consumer) | Rack shell (`AnalogLFO.cpp` atomics + `drawPhaseDot`) | DSP telemetry | Bug is the shell storing **raw** `t.swingFrac` instead of the gated value; GUI-thread read |
| Patch JSON parse (BUG-04) | Rack shell (`AnalogLFO.cpp::dataFromJson`) | — | Rack `json_t*` I/O; not in the Rack-free core, cannot link `jansson` in `make test` |
| Regression/unit tests | Test harness (`tests/*.cpp` + doctest) | DSP core | doctest TUs over `forge::*` headers + `BlockDriver` |

## Standard Stack

This is an existing codebase with a frozen toolchain. No new dependencies — the entire phase is edits to existing files plus new `tests/*.cpp` translation units.

### Core (already present — DO NOT re-introduce)
| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| doctest | 2.4.11 (vendored `tests/doctest.h`) | Headless test framework | Single-header, drop-in; pinned in Phase 22 [VERIFIED: tests/doctest.h header, make test output] |
| Rack-SDK | `../Rack-SDK` (relative) | Plugin build only — NOT linked by `make test` | `make test` is Rack-free by design (TEST-01/D-09) [VERIFIED: Makefile L18-21] |
| Plugin toolchain | system `$(CXX)`, `-std=c++17 -O2 -ffp-contract=off` | Compiles tests | `-ffp-contract=off` for cross-platform bit stability [VERIFIED: Makefile TEST_CXXFLAGS] |

### Supporting (existing extracted DSP units the fixes touch)
| Header | Purpose | This phase's edit |
|--------|---------|-------------------|
| `src/dsp/RatioTable.hpp` | `RATIO_TABLE[15]`, `shouldReset(idx,beat)` | BUG-02: add `BEATS_PER_ALIGN[15]`, swap the divisor line |
| `src/dsp/ClockTracker.hpp` | Clock FSM (outlier rejection at L117-127) | BUG-01: add consecutive-outlier counter + recovery |
| `src/dsp/Swing.hpp` | `SWING_FRACTIONS[6]`, `swingPhaseMultiplier` | BUG-03: no change needed — gate already correct here |
| `src/dsp/LfoCore.hpp` | Orchestrator + `Telemetry` | BUG-03: telemetry already correct; no change (verify) |
| `src/AnalogLFO.cpp` | Rack shell | BUG-03 (L316 store), BUG-04 (L243-247 parse) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `BEATS_PER_ALIGN[15]` table | Compute `q` from `RATIO_TABLE` at runtime via Fraction reduction | Table is `constexpr`, branch-free, matches CODE-REVIEW exactly; runtime reduction adds float→fraction fragility. Use the table. |
| `try/catch` around `std::stoull` (BUG-04) | `std::strtoull` (non-throwing, sets `errno`/endptr) | Both valid; CODE-REVIEW #4 names both. `strtoull` avoids exception machinery and is the lighter touch. Recommend `strtoull` with endptr + range check, OR a tight `try/catch` — planner picks one. |

**Installation:** None. No `npm`/`pip`/`cargo` — this is a C++ plugin with a vendored single-header test framework already in-tree.

## Package Legitimacy Audit

> Not applicable — this phase installs **zero** external packages. The only third-party code is `tests/doctest.h`, vendored and pinned to release tag 2.4.11 in Phase 22 (already in-tree, already CI-green). No registry fetch, no `postinstall`, no new dependency of any ecosystem occurs in Phase 23.

**Packages removed due to slopcheck [SLOP] verdict:** none (no packages)
**Packages flagged as suspicious [SUS]:** none (no packages)

## Architecture Patterns

### System Architecture Diagram

```
                          make test  (Rack-FREE — links NO libRack)
                                │
                                ▼
   doctest TUs ────────────────────────────────────────────────┐
   (tests/*.cpp)                                                │
     │  drive                                                   │
     ▼                                                          │
  ┌─ BlockDriver.hpp ──────────┐     ┌─ direct header drive ───┐│
  │  run(N, inputAt) over      │     │  ClockTracker.step(...)  ││
  │  forge::LfoCore.step()     │     │  shouldReset(idx,beat)   ││
  └──────────┬─────────────────┘     │  swingPhaseMultiplier()  ││
             │ per-sample             └──────────┬───────────────┘│
             ▼                                   ▼                │
  ┌─ forge::LfoCore (orchestrator, LfoCore.hpp) ─────────────────┐│
  │   clock: ClockTracker  ──► outlier reject (BUG-01)           ││
  │   shouldReset (RatioTable) ──► BEATS_PER_ALIGN (BUG-02)      ││
  │   Swing.swingPhaseMultiplier ──► isClocked gate (BUG-03 src) ││
  │   Telemetry{swingFrac,isClocked,displayPhase}  ─────────────►┼┘ (asserts)
  └─────────────────────────────────────────────────────────────┘

   ───────────────────  RACK SHELL (NOT in make test) ───────────────────
   src/AnalogLFO.cpp  process()  ──► reads core.tel ──► display atomics
     │                                   │
     │ BUG-03 consumer:                  ▼
     │   displaySwingFraction.store(  t.isClocked ? t.swingFrac : 0.5f )
     │   drawPhaseDot() (GUI thread) reads that atomic, warps the dot
     │
     └ BUG-04: dataFromJson(json_t*) ── json_is_string guard (present)
                                      └─ std::stoull (STILL THROWS — fix here)
```

Data flow: doctest cases enter via `BlockDriver` (whole-core, per-sample) **or** call the extracted pure functions directly. The shell is exercised only by the human in-Rack audition and manual UAT — it is unreachable from `make test` (no `jansson`, no `APP->`, no GL).

### Recommended Project Structure
```
src/dsp/
├── ClockTracker.hpp     # BUG-01 edit (consecutive-outlier counter)
├── RatioTable.hpp       # BUG-02 edit (BEATS_PER_ALIGN table)
├── Swing.hpp            # BUG-03: read-only (gate already correct)
└── LfoCore.hpp          # BUG-03: read-only (telemetry already correct)
src/
└── AnalogLFO.cpp        # BUG-03 consumer (L316), BUG-04 (L243-247)
tests/
├── test_dsp_stateful.cpp   # extend: BUG-01 re-lock regression
├── test_dsp_units.cpp      # extend: BUG-02 alignment table, swing math, waveshape ranges
├── test_regression.cpp     # NEW (recommended): the TEST-05 fail-before/pass-after pins
└── (BlockDriver.hpp, doctest.h, golden/ — unchanged)
```

### Pattern 1: Strict TDD "fails-on-old / passes-on-new" for an in-place header fix
**What:** The fixes edit `ClockTracker.hpp` / `RatioTable.hpp` in place. A regression test asserts the *fixed* behavior; run it against the unmodified header first to prove it goes RED.
**When to use:** Every fix #1–#3 (TEST-05).
**Procedure (encode as task ordering):**
```
1. Write the regression test asserting POST-FIX behavior.
2. make test  →  MUST FAIL (the new test red, all others green). Record the red.
3. Apply the minimal source fix.
4. make test  →  MUST PASS (all green).
5. Commit test + fix together; the commit message cites the red→green transition.
```
**Why this matters here:** because the fix and test land in the same tree, "fails before" must be demonstrated as a deliberate intermediate `make test` run, not inferred. The planner should make step 2 an explicit verifiable action (e.g. a task whose `<verify>` is "new case is the only failure"), or split test-authoring and fix into two tasks within the same plan wave.

### Pattern 2: Driving the clock FSM headlessly (BUG-01)
**What:** `test_dsp_stateful.cpp` already has `driveClock()` and `clockInterval()` helpers that feed a square wave and a single-edge pulse into `ClockTracker::step(clkV, dt, connected, ratioIdx)`.
**Example (existing, extend this):**
```cpp
// Source: tests/test_dsp_stateful.cpp L84-100 (existing re-acquire test)
forge::ClockTracker ct;
driveClock(ct, 120.0, 4.0, 48000.0, 7);      // lock at 0.5s period
REQUIRE(ct.clockState == forge::LOCKED);
// BUG-01 regression: a SUSTAINED >3x SPEEDUP (the lockout case the timeout can't escape)
// must re-acquire via the consecutive-outlier counter, not stall forever.
driveClock(ct, 480.0, 4.0, 48000.0, 7);      // 4x faster: period 0.125s
CHECK(ct.smoothedPeriod < 0.25f);            // moved OFF the stale 0.5s — proves re-lock
```
**Critical:** the existing L84 test only covers a *slowdown* (which the 1s timeout floor already rescues). The genuine BUG-01 lockout is the **speedup** case where every fast edge is rejected as an outlier AND resets the timer so the timeout never fires. The new regression test MUST exercise a sustained >3× speedup (and the narrow fast-clock slowdown band noted in CODE-REVIEW #1).

### Pattern 3: BEATS_PER_ALIGN table swap (BUG-02)
**What:** Replace the `round(1/RATIO_TABLE[idx])` divisor in `shouldReset` with a per-ratio table.
**Example:**
```cpp
// src/dsp/RatioTable.hpp — slots into the existing // FUTURE (P23) marker (L56)
// VERIFIED mathematically: reset every q beats (p/q in lowest terms) ⇒ integer cycles.
static constexpr int BEATS_PER_ALIGN[15] = {
//  /16 /8 /6 /4 /3 /2  /1.5 x1 x1.5 x2 x3 x4 x6 x8 x16
     16, 8, 6, 4, 3, 2,  3,   1,  2,  1, 1, 1, 1, 1, 1
};
inline bool shouldReset(int ratioIdx, int beatCount) {
    if (ratioIdx < 0) return true;                 // unlocked: every beat (unchanged)
    return beatCount >= BEATS_PER_ALIGN[ratioIdx]; // div AND non-int ratios handled uniformly
}
```
**Verified change set:** only idx 6 (2→3) and idx 8 (1→2) differ from current behavior. All 13 other ratios are bit-identical, so the change is provably surgical. [VERIFIED: python Fraction computation, this session]

### Anti-Patterns to Avoid
- **Re-implementing the reset divisor inside `ClockTracker.hpp`:** the tracker already delegates to `forge::shouldReset` (ClockTracker.hpp L172, CR-03). Keep the single home — patching only `RatioTable.hpp` keeps the FSM and table in sync.
- **Editing `Swing.hpp` for BUG-03:** the `isClocked` gate there is already correct. The bug is purely the *shell consumer* storing the raw value. Do not touch the DSP math.
- **Regenerating golden fixtures for BUG-02:** the goldens are **free-run only** (`clkConnected=false`, swing Straight) — see below. Adopting the table does NOT invalidate them.
- **Asserting absolute float periods bit-exactly for clock tests:** use `Approx`/inequality bands (the existing tests do: `smoothedPeriod > 1.5f`), never `==` on EMA-smoothed periods.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Square-wave clock injection | Custom per-test edge loops | `BlockDriver::clockedScenario(sr,bpm,base)` + `driveClock`/`clockInterval` helpers | Already exist; tested; correct 50% duty rising-edge semantics [VERIFIED: BlockDriver.hpp L59, test_dsp_stateful.cpp L24-61] |
| Ratio→beats reduction | Float→fraction reducer at runtime | `constexpr BEATS_PER_ALIGN[15]` | Math is fixed and known; the table is the CODE-REVIEW spec |
| Non-throwing hex parse | Manual hex-char loop | `std::strtoull` (non-throwing) OR tight `try/catch` round `std::stoull` | Both are the CODE-REVIEW-named fixes; don't write a parser |
| Bit-exact replay infra | New golden mechanism | Existing `tests/golden/*.f32` + `test_golden.cpp` | Unaffected by these fixes (free-run goldens) |

**Key insight:** Phase 22 deliberately shaped the extracted headers so these four fixes are minimal, local edits. The biggest risk is *over*-building — every "Don't Build" item above already has a tested home.

## Runtime State Inventory

> This is a bug-fix phase, not a rename/migration. Included for completeness because BUG-02 changes user-audible behavior and BUG-04 touches persistence.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | Patch JSON stores `spreadSeed0`/`spreadSeed1` (hex strings) + `swingIndex` (int). BUG-04 is a *read*-path crash on malformed values — no schema change, no migration. | Code edit only (harden the parse). No existing-patch migration. |
| Live service config | None — desktop plugin, no external services. | None — verified (no network/service deps in `src/AnalogLFO.cpp`). |
| OS-registered state | None. | None — verified. |
| Secrets/env vars | None. | None — verified. |
| Build artifacts | Stale **installed** plugin dir is a known false-negative source for in-Rack verification (audition gate). `~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/` can hold a stale `plugin.dylib`/`res`. | Audition task MUST `make install` + hash-compare `shasum plugin.dylib` vs installed, then fully quit+relaunch Rack BEFORE listening. [VERIFIED: memory vcv_build_install_workflow.md] |

**Golden-fixture impact of BUG-02 (explicit):** The golden `.f32` files were captured with `clkConnected=false, swing_index=0` (free-run, Straight) — see `tests/golden/freerun_seeds.txt` L13-19. `shouldReset` is only reached on a clock edge in clocked mode. **Therefore adopting BEATS_PER_ALIGN cannot change any golden sample and no regeneration is required.** [VERIFIED: freerun_seeds.txt, test_golden.cpp scenario]

## Common Pitfalls

### Pitfall 1: BUG-01 test that only covers slowdown (false sense of coverage)
**What goes wrong:** The existing re-acquire test (test_dsp_stateful.cpp L84) slows the clock down — which the 1s timeout floor already rescues even on the OLD code. A BUG-01 regression test that only slows down will pass on the unfixed code (no RED), violating TEST-05.
**Why it happens:** The genuine lockout is the *speedup* path: fast edges are rejected as outliers AND each rejected edge still ran `clockTimer.reset()` upstream (ClockTracker.hpp L105 runs before the L118 outlier check), so the no-pulse timeout never fires.
**How to avoid:** The BUG-01 regression MUST drive a sustained >3× **speedup** (e.g. 120→480 BPM) and assert `smoothedPeriod` moves to the new fast period. Also cover the fast-clock narrow slowdown band (smoothed period < ~0.33s, where `timeout = max(1.0, …)` floors at 1s).
**Warning signs:** New test passes against the current header (no RED step) — that means it isn't exercising the lockout.

### Pitfall 2: Consecutive-outlier counter semantics (off-by-one / reset placement)
**What goes wrong:** Wrong counter reset point leaves a permanent stall or makes the tracker jittery (snaps on a single genuine outlier).
**Design constraints (from CODE-REVIEW #1 + the live FSM):**
- Threshold: 2–3 consecutive outliers before declaring a real tempo change (CODE-REVIEW says "2-3"). **Recommend 3** for robustness against a single glitch edge; the regression test should pin whatever is chosen.
- On reaching threshold: either snap `smoothedPeriod` to the raw measurement OR drop to `ACQUIRING` and re-learn. **Recommend drop-to-ACQUIRING** (re-uses the existing fast-track re-acquire at L130-137, smaller behavioral surface).
- Reset the counter to 0 on ANY accepted (non-outlier) edge.
- The counter only increments inside the `clockState == LOCKED && isOutlier` branch (L118-127) — that early-`return` currently discards the edge; the counter increment + threshold check must happen *before* that return.
- Interaction with `clockTimer.reset()`: the timer was already reset at L105 for this edge. If you drop to ACQUIRING, the next edge's `rawPeriod` is measured cleanly from here — good. Do not double-reset.
**How to avoid:** Mirror the existing structure; add `int consecutiveOutliers = 0;` member; increment in the outlier branch, check threshold, on threshold set `clockState = ACQUIRING; clockEdgeCount = 1; consecutiveOutliers = 0;` (or snap), else early-return as today. Reset `consecutiveOutliers = 0` on every accepted edge path.

### Pitfall 3: BUG-04 is only HALF fixed in the live tree
**What goes wrong:** The `json_is_string` guard (AnalogLFO.cpp:243) is already committed, which can look "done." But `std::stoull` (L244-245) still throws on a string that *is* a string but isn't valid hex, or is too long.
**Empirical proof (this session):**
```
std::stoull("zzzz", …, 16)                  → throws invalid_argument ("no conversion")
std::stoull("FFFFFFFFFFFFFFFFFFFF", …, 16)  → throws out_of_range
std::stoull("", …, 16)                      → throws invalid_argument
```
All three are reachable from a hand-edited patch and currently propagate uncaught → Rack crash on load.
**How to avoid:** Wrap L244-245 in `try/catch(const std::exception&){ /* keep existing seed */ }` OR switch to `strtoull` with endptr/`errno` checks. On any parse failure, **do not** call `initComponentSpread()` — fall through keeping the constructor-seeded spread (the "existing seed" fallback CODE-REVIEW #4 names).
**Warning signs:** A BUG-04 test that only feeds a non-string node (e.g. an integer) passes on the current code — because the type guard already catches that. The regression test MUST feed a **malformed string** (`"zzzz"`, an over-long hex) to hit the still-open `stoull` throw.

### Pitfall 4: BUG-04 and BUG-03 are NOT reachable from `make test` (jansson + GL)
**What goes wrong:** Trying to call `dataFromJson` or `drawPhaseDot` from a doctest TU fails to link — they need `jansson` (`json_t*`) and `NVGcontext*`/`APP->`, none of which the Rack-free harness provides.
**Why it happens:** Both live in `struct AnalogLFO : rack::Module` in `src/AnalogLFO.cpp`, which cannot compile under `make test` (same constraint that drove the Phase 22 extraction).
**How to avoid — two options for the planner:**
- **(a) Extract a pure helper** for BUG-04: a free function `bool parseSeedHex(const char* s, uint64_t& out)` in a Rack-free header (e.g. `src/dsp/PatchParse.hpp`) returning success/failure, unit-tested headlessly; `dataFromJson` calls it after the `json_is_string` guard. This makes BUG-04 a TEST-05-pinnable regression. **Recommended** — it is the smallest extraction and directly testable.
- **(b) Manual-only verification** for the shell consumer side of BUG-03 + the `dataFromJson` wiring: documented in-Rack steps (load a hand-corrupted patch; set Medium swing + unplug clock and watch the dot). BUG-03's *producer* (the gated `swingFrac` value) is already covered by `Swing` unit tests; the *consumer* mismatch is a one-line shell store that is visual-only.
- **Recommended split:** BUG-03 — assert the **gate value** headlessly (`swingPhaseMultiplier(..., isClocked=false) == 1.0` already tested; add a `LfoCore` telemetry check that `tel.swingFrac` reflects the warp source) AND fix the shell store (L316 → `t.isClocked ? t.swingFrac : 0.5f`) with a manual UAT note. BUG-04 — extract `parseSeedHex` (option a) for a real headless regression.

### Pitfall 5: Real-time / atomic concerns on the shell edits
**What goes wrong:** BUG-03's fix changes what value `process()` (audio thread) stores into `displaySwingFraction` (read by GUI thread in `drawPhaseDot`). The store is already `std::memory_order_relaxed` on a `std::atomic<float>` — correct. Don't introduce a non-atomic intermediate.
**How to avoid:** Change only the *value* stored at L316 from `t.swingFrac` to `t.isClocked ? t.swingFrac : 0.5f` (mirroring the buffer-gen gate already at L332). No new shared state, no lock, no ordering change.
**Note:** The known pre-existing `swingIndex` GUI→audio non-atomic write (STATE.md Deferred) is OUT OF SCOPE — do not "fix" it here.

### Pitfall 6: Determinism of clock-jump tests
**What goes wrong:** Clock tests that depend on EMA-smoothed float periods compared with `==` are brittle across platforms.
**How to avoid:** Assert inequality bands / `Approx` (as the existing suite does). Clock-path tests touch no `std::normal_distribution` (that's the drift path), so they are bit-stable across OSes — but still prefer bands over exact periods because EMA convergence depends on edge count. Seed/drift are irrelevant to clock FSM tests; keep drift=0 to isolate.

### Pitfall 7: The audition-before-code ordering gate (ROADMAP-referenced)
**What goes wrong:** Implementing the BEATS_PER_ALIGN table (and its regression test) before the human in-Rack listening audition violates Success Criterion 3 — the decision (keep-current vs adopt-table) must be **logged first**. An autonomous agent must NOT self-decide x1.5 groove desirability.
**Why it matters:** x1.5's every-beat retrig may be a *desirable* groove; it is mathematically inconsistent but musically intentional-sounding. Only a human listening test resolves it. The change is user-audible and irreversible-feeling once shipped.
**How to encode (BLOCKING, non-autonomous task):**
1. A `checkpoint:human-verify` task (no code) that:
   - Builds + installs the CURRENT (unfixed) plugin: `make RACK_DIR=../Rack-SDK && make install RACK_DIR=../Rack-SDK`.
   - **Flushes stale install**: `shasum plugin.dylib` vs the installed copy; `rsync` res + copy dylib if they differ; fully quit + relaunch Rack (memory: vcv_build_install_workflow.md — false-negative trap).
   - Auditions x1.5 and ÷1.5 in Rack against a clock; compares current every-beat retrig vs the proposed table cadence.
   - **Records the decision** in `.planning/STATE.md` under `### Decisions` as a dated `- [Phase 23]: x1.5/÷1.5 audition — DECISION: {keep-current | adopt-table} — rationale: …` entry (this is the project's "milestone log" — verified: STATE.md L53-58 and the existing `- [Phase ?]:` decision entries are the logging convention).
2. ONLY after that entry exists may the Wave-C implementation + regression test tasks run.
**Test must stay valid under EITHER outcome:** Write the BUG-02 regression as an assertion over `shouldReset`'s reset cadence that reads the **decided** table. If "keep-current": the test pins the current `round(1/ratio)` cadence (idx 6 resets every 2, idx 8 every 1). If "adopt-table": it pins `{…,3,…,2,…}`. Structure the test so the expected-cadence array is a single named constant the implementer sets per the logged decision — the test then proves *whatever was decided* is what `shouldReset` does (no mid-cycle truncation if adopted; preserved behavior if kept). The 13 unchanged ratios are asserted identically in both cases.

### Pitfall 8: "fails-before" must be demonstrated, not assumed (TEST-05)
**What goes wrong:** Because fix + test land together, a reviewer can't see the RED. The criterion is literally "fails before the fix and passes after."
**How to avoid:** For each of BUG-01/-03/-04 (and BUG-02 post-audition), the plan makes the RED an explicit, recorded `make test` run (e.g. task A writes the test and asserts it's the only failure; task B applies the fix and asserts all-green; both in the same plan, committed together). The SUMMARY should cite the red→green for each fix.

## Code Examples

### BUG-02 regression: deterministic reset-cadence over `shouldReset`
```cpp
// tests/test_regression.cpp (NEW)  — valid under EITHER audition outcome.
// Set EXPECTED per the logged STATE.md decision (adopt-table shown).
TEST_CASE("BUG-02: ratio alignment cadence (per logged audition decision)") {
    // beats-per-reset the implementation MUST produce, indexed by ratioIdx.
    // adopt-table:  idx6 /1.5 = 3, idx8 x1.5 = 2; keep-current: idx6=2, idx8=1.
    static const int EXPECTED[15] = {16,8,6,4,3,2, 3, 1, 2, 1,1,1,1,1,1};
    for (int idx = 0; idx < 15; ++idx) {
        int period = EXPECTED[idx];
        for (int b = 1; b < period; ++b)
            CHECK_FALSE(forge::shouldReset(idx, b));   // no early (mid-cycle) reset
        CHECK(forge::shouldReset(idx, period));        // resets exactly on the boundary
    }
}
```

### BUG-01 regression: re-lock after a sustained >3× speedup
```cpp
// tests/test_dsp_stateful.cpp (extend) — uses the existing driveClock helper.
TEST_CASE("BUG-01: re-acquires after a sustained >3x SPEEDUP (no lockout)") {
    forge::ClockTracker ct;
    driveClock(ct, 120.0, 4.0, 48000.0, 7);   // lock at period 0.5s
    REQUIRE(ct.clockState == forge::LOCKED);
    driveClock(ct, 540.0, 4.0, 48000.0, 7);   // 4.5x faster -> period ~0.111s
    // Consecutive-outlier counter must let it re-acquire the FAST period; the OLD
    // code stays stuck at ~0.5s forever (this is the RED-before assertion).
    CHECK(ct.smoothedPeriod < 0.25f);
}
```

### BUG-04 regression (with the recommended `parseSeedHex` extraction)
```cpp
// src/dsp/PatchParse.hpp (NEW, Rack-free): non-throwing hex parse.
inline bool parseSeedHex(const char* s, uint64_t& out) {
    if (!s || !*s) return false;
    char* end = nullptr; errno = 0;
    unsigned long long v = std::strtoull(s, &end, 16);
    if (errno == ERANGE || end == s || *end != '\0') return false;
    out = (uint64_t)v; return true;
}
// tests/test_regression.cpp
TEST_CASE("BUG-04: malformed seed string never throws, signals failure") {
    uint64_t v = 0;
    CHECK_FALSE(forge::parseSeedHex("zzzz", v));                   // non-hex
    CHECK_FALSE(forge::parseSeedHex("FFFFFFFFFFFFFFFFFFFF", v));   // out of range
    CHECK_FALSE(forge::parseSeedHex("", v));                       // empty
    CHECK(forge::parseSeedHex("C0FFEE", v)); CHECK(v == 0xC0FFEEull); // valid round-trips
}
// AnalogLFO.cpp dataFromJson: after json_is_string guard, call parseSeedHex;
// on false, keep the existing seed (do NOT call initComponentSpread()).
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Inline DSP inside `rack::Module` | Extracted Rack-free `src/dsp/*.hpp` + `make test` | Phase 22 | These four bugs are now (mostly) headless-testable; the extraction explicitly pre-marked the P23 fix sites |
| `round(1/ratio)` divisor (div ratios only; non-int ratios fall through) | `BEATS_PER_ALIGN[15]` (uniform, lowest-terms q) | Phase 23 (this) | Fixes x1.5/÷1.5 truncation — **pending audition** |
| `std::stoull` after presence check | `json_is_string` guard (done) + non-throwing parse (TODO) | Phase 23 (this) | Closes the patch-load crash |

**Deprecated/outdated:** none relevant. The CODE-REVIEW line numbers (`commit 79be789` + uncommitted 20-03 tweaks) have DRIFTED — the live `src/AnalogLFO.cpp` is 1132 lines and post-extraction. Verified live anchors: `dataFromJson` L238-252, `displaySwingFraction.store` L316, buffer-gate L332, `drawPhaseDot` L575-582. The DSP-core bugs (BUG-01/-02/-03-src) now live in `src/dsp/*.hpp`, NOT at the CODE-REVIEW line numbers.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Consecutive-outlier threshold = 3 (vs 2) is the better default | Pitfall 2 | Low — either is acceptable per CODE-REVIEW; the regression test pins the chosen value |
| A2 | Drop-to-ACQUIRING is cleaner than snap-to-raw for BUG-01 recovery | Pitfall 2 | Low — both satisfy "no permanent lockout"; reuses existing fast-track path |
| A3 | `strtoull`-with-endptr is the preferred BUG-04 fix over `try/catch` | Stack / Code Examples | Low — both are CODE-REVIEW-named; functionally equivalent |
| A4 | Extracting `parseSeedHex` (option a) is preferred over manual-only for BUG-04 | Pitfall 4 | Medium — affects whether BUG-04 gets an automated TEST-05 pin; recommend confirming in discuss/plan |
| A5 | The "milestone log" = STATE.md `### Decisions` section | Pitfall 7 | Medium — verified against STATE.md convention, but the user may intend the phase SUMMARY or a dedicated audition log; confirm the exact sink at plan time |
| A6 | Manual in-Rack audition is the only human gate; all else is automated | Validation Architecture | Low — matches ROADMAP "Human verification gate: yes" scope |

## Open Questions

1. **BUG-04 testability path — extract `parseSeedHex` vs manual-only?**
   - What we know: `dataFromJson` can't be reached headlessly; the throw is in `std::stoull`.
   - What's unclear: whether the team wants the small extraction (A4) to earn an automated TEST-05 pin for BUG-04, or accepts manual verification (BUG-04 is "medium priority robustness," and TEST-05 names only fixes #1–#3 explicitly).
   - Recommendation: extract `parseSeedHex` — it is a ~6-line Rack-free helper that turns a crash-class bug into a deterministic test, with negligible blast radius. TEST-05's literal scope is #1–#3, so BUG-04's pin is a bonus, not a requirement.

2. **Exact sink for the audition decision ("milestone log").**
   - What we know: STATE.md `### Decisions` holds the project's dated decision entries (verified).
   - What's unclear: whether the user prefers that, the phase SUMMARY, or a new `.planning/phases/23-*/23-AUDITION.md`.
   - Recommendation: log to STATE.md `### Decisions` (matches the existing `- [Phase N]:` convention) AND echo into the phase SUMMARY for traceability. Confirm at discuss/plan.

3. **BUG-02 regression under "keep-current."**
   - What we know: the test must stay valid either way (Pitfall 7).
   - Recommendation: parameterize the `EXPECTED[15]` array; the implementer sets it from the logged decision. Documented in the code example above.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `make` + system `c++` (C++17) | `make test`, plugin build | ✓ | system clang (tests built & ran this session) | — |
| doctest 2.4.11 | test suite | ✓ | vendored `tests/doctest.h` | — |
| `../Rack-SDK` | plugin build + in-Rack audition only | assumed ✓ (build memory documents it) | — | `make test` does NOT need it |
| VCV Rack 2 (desktop app) | BUG-02 audition gate ONLY | human-operated | — | none — audition is inherently human/manual |

**Missing dependencies with no fallback:** none for the automated path. The audition requires the human operator's Rack install (out of agent control by design).
**Missing dependencies with fallback:** `make test` is fully Rack-free and ran green this session (35 cases, 1.6M assertions, ~5s) — the automated half of the phase needs nothing beyond a C++17 compiler.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | doctest 2.4.11 (vendored `tests/doctest.h`) |
| Config file | none — `make test` recipe in `Makefile` (TEST_-namespaced, Rack-free) |
| Quick run command | `make test` |
| Full suite command | `make test` (single binary, all TUs; multi-rate cases included) |
| Estimated runtime | ~5 s (confirmed this session) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BUG-01 | Re-lock after sustained >3× speedup / fast-clock slowdown band; no permanent lockout | unit (stateful FSM) | `make test` (`test_dsp_stateful.cpp`) | ✅ extend existing |
| BUG-02 | Ratio→beats reset cadence per logged audition decision; no mid-cycle truncation | unit (table) | `make test` (`test_regression.cpp`) | ❌ Wave 0 (new TU) — **gated by audition** |
| BUG-03 (producer) | Swing warp gated off in free-run (`swingPhaseMultiplier(...,false)==1.0`; `tel.swingFrac` source) | unit | `make test` (`test_dsp_units.cpp`) | ✅ extend / ✅ exists |
| BUG-03 (consumer) | Phase dot uses gated display swing | manual UAT | in-Rack (Medium swing + unplug clock; dot tracks trace) | ❌ manual-only |
| BUG-04 | Malformed seed string never throws; falls back to existing seed | unit | `make test` (`test_regression.cpp` + `parseSeedHex`) | ❌ Wave 0 (new helper+TU) |
| TEST-03 | Waveshape output ranges, ratio/alignment table, outlier recovery, swing math | unit | `make test` (`test_dsp_units.cpp` + `test_dsp_stateful.cpp`) | ⚠️ partial — see gaps |
| TEST-05 | Each fix #1–#3 fails-before / passes-after | unit (RED→GREEN procedure) | `make test` (explicit red step per fix) | ❌ Wave 0 (procedure + cases) |

### Success-Criterion → Validation Mapping
| Success Criterion | Validation |
|-------------------|------------|
| SC1 clock re-acquires after >3×/<⅓× jump | `test_dsp_stateful.cpp` BUG-01 speedup + fast-slowdown cases (headless) |
| SC2 phase dot tracks (free-run+swing) AND patch survives malformed JSON | `Swing`/telemetry headless + shell store fix (manual UAT for the dot); `parseSeedHex` headless for JSON |
| SC3 AUDITION GATE logged before code | **single human gate** — `checkpoint:human-verify` task writing the STATE.md `### Decisions` entry (Pitfall 7) |
| SC4 x1.5/÷1.5 align per decision, deterministic regression | `test_regression.cpp` BUG-02 cadence test reading the decided `EXPECTED[15]` |
| SC5 TEST-03 coverage + every fix #1–#3 pinned | combined unit suite; RED→GREEN per fix (TEST-05) |

### Sampling Rate
- **Per task commit:** `make test`
- **Per wave merge:** `make test` + `make RACK_DIR=../Rack-SDK` (plugin still builds unchanged)
- **Phase gate:** full suite green + plugin builds + audition decision logged, before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `tests/test_regression.cpp` — NEW TU: BUG-02 cadence (post-audition), BUG-04 `parseSeedHex` pins
- [ ] `src/dsp/PatchParse.hpp` — NEW: non-throwing `parseSeedHex` (enables BUG-04 headless pin)
- [ ] `tests/test_dsp_stateful.cpp` — extend: BUG-01 **speedup** re-lock + fast-clock slowdown band
- [ ] `tests/test_dsp_units.cpp` — extend TEST-03: waveshape output-range coverage (strict post-scale ±5V is in `test_invariants.cpp`; the unit-level shape-range scaffold at L101 widened to the full grid), full ratio/alignment-table cases, swing-math edges
- [ ] No framework install needed — doctest + `make test` already in place (Phase 22)

*(BUG-03 consumer + the `dataFromJson` wiring are manual-only — listed in Manual-Only below, not Wave 0 automated.)*

### Manual-Only Verifications
| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| x1.5/÷1.5 groove desirability (audition) | BUG-02 / SC3 | Subjective musical judgment; gates the code change | Build+install current plugin, flush stale install (hash-compare dylib, relaunch Rack), audition x1.5 & ÷1.5 vs clock, log decision to STATE.md `### Decisions` |
| Phase dot tracks trace (free-run + Medium/Heavy swing) | BUG-03 consumer | GUI render (NanoVG); not reachable from Rack-free harness | Select Medium swing, unplug clock; confirm dot sits on the trace/output, not swing-warped ahead of it |
| `dataFromJson` end-to-end with a hand-corrupted patch | BUG-04 wiring | Needs `jansson`/full module load | Hand-edit a `.vcv` patch: set `spreadSeed0` to `"zzzz"`; load → Rack must not crash; module keeps a valid seed |

## Security Domain

> `security_enforcement` is not set in `.planning/config.json` (treated as enabled). This is an offline desktop audio plugin with one untrusted input surface: the patch file.

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No auth surface (local plugin) |
| V3 Session Management | no | No sessions |
| V4 Access Control | no | No access boundaries |
| V5 Input Validation | **yes** | Patch JSON is untrusted input — BUG-04 is exactly an input-validation/robustness fix: type-guard (`json_is_string`, present) + non-throwing parse + range check + safe fallback |
| V6 Cryptography | no | Seeds are RNG state, not secrets; no crypto |

### Known Threat Patterns for {VCV Rack C++ plugin / patch loading}
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed/hand-edited patch → uncaught exception → host crash (BUG-04) | Denial of Service | Type-guard + non-throwing parse (`strtoull`/guarded `stoull`), fall back to a valid default seed; never construct `std::string` from a NULL `json_string_value` |
| NULL `json_string_value` deref (UB) | Tampering / DoS | `json_is_string()` before `json_string_value()` (already in tree at L243) |
| Out-of-range / over-long seed value | DoS | `errno==ERANGE`/`endptr` check (`strtoull`) or `catch(out_of_range)` |

No network, filesystem-write, or privilege surface is introduced by this phase.

## Sources

### Primary (HIGH confidence)
- `src/dsp/ClockTracker.hpp`, `RatioTable.hpp`, `Swing.hpp`, `LfoCore.hpp`, `RackCompat.hpp` — live extracted DSP core (read this session)
- `src/AnalogLFO.cpp` L238-252, L306-339, L575-582 — live shell bug sites (read this session)
- `tests/BlockDriver.hpp`, `test_dsp_stateful.cpp`, `test_dsp_units.cpp`, `test_invariants.cpp`, `test_golden.cpp`, `test_extraction.cpp`, `tests/golden/freerun_seeds.txt` — existing harness (read this session)
- `Makefile` (TEST_ target), `make test` run output (35/35 green) — this session
- `.planning/ROADMAP.md` L105-130, `.planning/REQUIREMENTS.md` L14-26, `.planning/STATE.md` (Decisions/milestone-log convention) — this session
- `CODE-REVIEW-FINDINGS.md` #1–#4 + "Suggested order of attack" — phase driver
- Empirical `std::stoull` throw test (this session) — confirms BUG-04 still open
- Python `Fraction` computation of BEATS_PER_ALIGN (this session) — verifies the table & its 2-cell delta
- memory `vcv_build_install_workflow.md` — stale-install flush for the audition gate

### Secondary (MEDIUM confidence)
- `.planning/phases/22-test-harness-foundation/22-VALIDATION.md` — Validation Architecture format precedent

### Tertiary (LOW confidence)
- none — all claims verified against live source or computed this session

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new deps; toolchain verified by a green `make test` this session
- Architecture: HIGH — DSP core read line-by-line; fix sites located and the table verified by computation
- Pitfalls: HIGH — BUG-04 half-fix, BUG-01 speedup-vs-slowdown, golden non-impact, and the audition gate all verified against live source/fixtures
- Audition gate / milestone log: MEDIUM — sink convention verified, but exact preferred location worth confirming at plan time (A5/Open Q2)

**Research date:** 2026-06-14
**Valid until:** 2026-07-14 (stable codebase; revisit if `src/dsp/*` or `dataFromJson` changes before planning)
```
