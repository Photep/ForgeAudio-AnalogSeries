---
phase: 31
slug: pitch-tuning-exponential-fm
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-29
---

# Phase 31 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Derived from `31-RESEARCH.md` § Validation Architecture (figures there are MEASURED, but D-18 requires this phase to re-measure and record its own).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest 2.4.11, vendored at `tests/doctest.h`; impl macro owned by `tests/main.cpp` |
| **Config file** | `Makefile:34-48` — `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off` |
| **Quick run command** | `make test` |
| **Full suite command** | `make test && make guards && make strict` |
| **Estimated runtime** | `make test` fast enough to run per task commit (2,616,112 assertions at baseline) |
| **Source globbing** | `TEST_SOURCES := $(wildcard tests/*.cpp)` — a new test file needs **zero** build wiring |
| **Regression floor (measured)** | `make test`: 72 cases / 2,616,112 assertions / 0 failed · `make guards`: PASS |

---

## Sampling Rate

- **After every task commit:** `make test`
- **After every plan wave:** `make test && make guards && make strict`
- **Before `/gsd-verify-work`:** full suite green, plus a real plugin link
- **Phase gate:** `make test` + `make guards` + `make strict` + plugin link + 3-OS CI matrix observed green **by SHA (never by recency)** + operator in-Rack UAT with a full `dist/` flush
- **Max feedback latency:** one `make test` run per task

---

## Per-Task Verification Map

Task IDs bind at plan time; rows are keyed by requirement so the planner can attach them.

| Requirement | Behavior | Test Type | Automated Command | File Exists | Status |
|-------------|----------|-----------|-------------------|-------------|--------|
| PITCH-01 | V/OCT tracks 1 V/oct off `C4 = 0 V` within **< 1 cent**, measured on the **OUTPUT** | unit (headless DSP) | `./build-test/test -tc="*v/oct tracking*"` | ❌ W0 — `tests/test_vco_pitch.cpp` | ⬜ pending |
| PITCH-01 | Secondary tier: `tel.freqHz` vs libm reference where crossings cannot resolve | unit | same file | ❌ W0 | ⬜ pending |
| PITCH-02 | COARSE `+n` octaves shifts measured pitch by exactly `n` octaves; full `±5` range reachable | unit | same file | ❌ W0 | ⬜ pending |
| PITCH-03 | FINE `±1` semitone shifts pitch by `±100 cents ± tolerance`; `fine / 12` conversion correct | unit | same file | ❌ W0 | ⬜ pending |
| PITCH-04 | `tel.freqHz <= kVcoNyquistGuardFrac * safeRate` for every input incl. hostile; clamp **fires** on a legitimate high note and the oscillator keeps sounding (D-10) | unit | `-tc="*Nyquist*"` | ⚠️ partial — `tests/test_vco_core.cpp:753` pins `freqNyquistBounded` symbolically but is driven by hostile *timing*, not hostile *pitch* | ⬜ pending |
| PITCH-05 | Non-regression: `phase` is `double`, `deltaPhase` computed with both casts | unit + review | existing `test_vco_core.cpp` scenario four (`phaseInRange`) | ✅ exists | ⬜ pending |
| FM-01 | Audio-rate FM input modulates pitch (non-trivial FM signal changes the measured pitch trajectory) | unit | `-tc="*exponential FM*"` | ❌ W0 | ⬜ pending |
| FM-02 | Attenuverter is bipolar: `fmAtten = -1` inverts `fmAtten = +1`; `fmAtten = 0` is a no-op | unit | same | ❌ W0 | ⬜ pending |
| FM-03 | **Summation identity:** static `fmVolts = V` at `fmAtten = 1` produces a **bit-exact** match to `pitchCV += V` with FM disconnected | unit | same | ❌ W0 | ⬜ pending |
| FM-03 | **Negative control:** a multiplicative stand-in (`freq *= exp2_taylor5(fm)`) must **FAIL** the same identity | unit (positive control) | same | ❌ W0 | ⬜ pending |
| D-09 | `fmConnected = false` makes the FM term a bit-exact no-op for any `fmVolts` / `fmAtten` | unit | same | ❌ W0 | ⬜ pending |
| D-14 | Hostile pitch volts (NaN, ±inf, ±1e30, ±200 V) never reach `exp2_taylor5` outside `[−kVcoMaxPitchVolts, +kVcoMaxPitchVolts]` | UBSan RED + standing behavioral case at the bound | `-tc="*hostile pitch*"` + one-shot UBSan probe | ❌ W0 | ⬜ pending |
| TEST-02 | **Phase gate** — the union of the PITCH-01 rows | unit | `make test` | ❌ W0 | ⬜ pending |
| guardrail | Six LFO goldens byte-identical; no frozen header edited | existing | `make test` (`test_golden.cpp`, `test_lfo_guardrail.cpp`) + `make guards` | ✅ exists | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## TEST-02 Sweep Design (constraints the tests must honor)

**Fixed inputs, primary tier:** `morph = 0.f`, `character = 0.f` — the crossing estimator's own error is ~100× lower at pure sine (measured 0.0002 cents vs 0.115 at `morph = 0.5`). Running the gate at any other morph measures the apparatus, not the oscillator.

**Upper bound per rate** = `min(clampCeilingVolts(sr), estimatorCeilingVolts(sr))` — **both derived at runtime, never hardcoded** (D-21). `estimatorCeilingVolts(sr) = log2((sr / 2.5) / 261.6256)`.

| Sample rate | Clamp ceiling | Estimator ceiling | Binding limit | Top test point |
|-------------|---------------|-------------------|---------------|----------------|
| 44100 | +6.3826 V | **+6.0745 V** | estimator | +6.0 V |
| 48000 | +6.5049 V | **+6.1968 V** | estimator | +6.0 V |
| 96000 | +7.5049 V | **+7.1968 V** | estimator | +7.0 V |

> The estimator binds at every rate and the clamp binds at none — but the `min()` is still required. A clamp-only bound admits `+6.5 V` at 48 kHz, which measures **−11.61 cents on a perfectly correct oscillator** (2.027 samples/cycle). Correct behavior, broken apparatus, failing like a pitch bug.

**Lower bound:** sweep `−5.0 V` (8.18 Hz) to the per-rate top in `0.5 V` steps at each rate. Add `−7.0 V` at one rate as a documented extreme point (measured `+0.00004` cents, needs an ~8 s window).

**Window rule:** `n = round(sr * max(0.25, 16.0 / f_expected))`. The `0.25 s` floor is adequate everywhere the sweep goes (worst measured `−0.0087` cents); shorter windows break at the top of the range.

**Tolerance:** fixed **0.05 cents** at every point and every rate — **no widening with samples-per-cycle** (D-20 forbids it). Provenance: 5.2× above the worst measured sweep point (0.00968 cents), 20× under the 1-cent requirement, ~5× above the DSP's own 0.0101-cent end-to-end worst — so the gate is dominated by oscillator error, not apparatus error. **D-18 requires this phase to re-measure and record its own figures rather than inherit this table.**

---

## Non-Vacuity Requirements (mandatory — the Phase 29/30 lesson)

Each must be implemented, not assumed:

1. **Measure the OUTPUT, not `tel.freqHz`,** on the primary tier (D-19 / Phase 30 D-16).
2. **Ground truth from libm** (`std::exp2` in double), never from `exp2_taylor5` (D-18) — otherwise the test proves the polynomial equals itself.
3. **Expectations one octave apart** across the grid, so an accumulator that latched a single frequency satisfies at most one point.
4. **`REQUIRE(nUp >= 8)` before any tolerance check**, so silent non-oscillation is a hard failure rather than a wrong number (catches the `-1.0` sentinel path).
5. **FM negative control** — a multiplying stand-in core must FAIL the FM-03 identity through the same helper. Model on `DeliberatelyBrokenSharedStateCore` (`tests/test_vco_core.cpp:316-366`): anonymous namespace, test TU only, **never under `src/`**.
6. **Clamp-boundary case proving the clamp FIRES** — just above the derived ceiling, `tel.freqHz` equals `kVcoNyquistGuardFrac * sampleRate` exactly **and** the output keeps oscillating (D-10: peaks flatten, not silence).

---

## Wave 0 Requirements

- [ ] `tests/test_vco_pitch.cpp` — TEST-02 tracking gate (PITCH-01), COARSE/FINE range cases (PITCH-02/03), FM summation identity + multiplicative negative control (FM-01/02/03, D-09), D-14 hostile-pitch standing case
- [ ] `tests/check_includes.sh` — one exact-path `VCO_SIDE_ALLOW` entry for the new test file, as an **explicit plan task with rationale**, not a gate-time discovery (Pitfall 6; the recurring Phase-30 landmine — `make guards` exits 1 without it)
- [ ] A **pitch-driven** Nyquist-clamp case (PITCH-04 / D-10) — the existing `freqNyquistBounded` pin at `tests/test_vco_core.cpp:753` is timing-driven and never observes the clamp firing on a legitimate high note
- [ ] D-14 RED evidence — one-shot UBSan probe (recommended) or a `Telemetry` field
- [ ] No framework install needed; no `conftest`-equivalent needed (`tests/main.cpp` owns the doctest impl macro)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Musical feel of COARSE/FINE and audio-rate FM in a real patch | PITCH-02, PITCH-03, FM-01 | Timbral/ergonomic judgement has no automated proxy | Operator in-Rack UAT after a **full `dist/` flush** (a partial flush silently leaves a stale plugin *version* — STATE.md Phase 30) |
| 3-OS CI matrix green | guardrail | Runs off-machine; MinGW `-std=c++11` leg catches what local clang masks | Confirm green **by commit SHA**, never by recency |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or a Wave 0 dependency
- [ ] Sampling continuity: no 3 consecutive tasks without an automated verify
- [ ] Wave 0 covers all ❌ MISSING references above
- [ ] No watch-mode flags
- [ ] All six non-vacuity requirements implemented
- [ ] This phase re-measured and recorded its own tracking figures (D-18) rather than inheriting the research table
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
