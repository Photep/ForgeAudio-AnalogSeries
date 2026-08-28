---
phase: 33
slug: hard-sync
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-08-28
---

# Phase 33 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Derived from `33-RESEARCH.md` § Validation Architecture. Every decibel figure carried here is an
> in-session **prototype prediction**, not a pinned threshold — the binding numbers come from the
> in-repo measurement against the real `forge::VcoCore`, per the D-08/D-09 measure-then-pin protocol.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest (header-only, vendored at `tests/doctest.h`) |
| **Config file** | `Makefile` — `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`; `TEST_SOURCES := $(wildcard tests/*.cpp)` |
| **Quick run command** | `make test` |
| **Full suite command** | `make test && make strict && make guards` |
| **Estimated runtime** | ~30–60 seconds (`make test`); the spectral sweep dominates and the sync sub-grid enlarges it |

**New wiring needed:** none for the build if the sub-grid lands in existing TUs (the `tests/*.cpp`
glob picks new files up automatically). Beyond that:

- **If a new test TU lands:** one `VCO_SIDE_ALLOW` line in `tests/check_includes.sh`.
- **For the D-13..D-16 renderer:** one `tools/*.cpp`, one `make` target, one entry in the Makefile's
  `MAKECMDGOALS` skip filter, **and** one `VCO_SIDE_ALLOW` line — `check_includes.sh` scans `tools/`
  too. All four are plan tasks with their own rationale, never gate-time discoveries (the Phase 31
  D-23 lesson).
- **For the POD:** two runtime-derived field feeds in `src/vco_compile_canary.cpp`.

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test && make strict && make guards`
- **Before `/gsd-verify-work`:** Full suite green **plus the CI MinGW link leg observed green on the
  exact commit**. `make strict` is `-fsyntax-only` and cannot substitute for it.
- **Max feedback latency:** 60 seconds

`make strict` is non-optional at wave boundaries: the VCV library toolchain is `-std=c++11` GCC, and
clang-masked C++17/ODR bugs fail there rather than locally. This phase's Task 1 exists because
`(int)NaN` measures `0` on the arm64 development host and `INT_MIN` under x86 `cvttss2si` on the
builds that actually ship.

---

## Per-Task Verification Map

Task IDs are assigned by the planner; this map is keyed by decision/requirement until plans exist.
Every row must resolve to at least one `<automated>` verify in a PLAN.md task.

| Requirement / Decision | Behavior | Test Type | Automated Command | File Exists | Status |
|------------------------|----------|-----------|-------------------|-------------|--------|
| D-04 / CR-01 | `MorphBlep::step` does not write `W[segment]` out of bounds for a negative or NaN `morph` | unit + **one-shot** ASan probe | `make test` (`tests/test_morph_blep.cpp`); ASan RED run once, **not wired** into `make test`/`guards`/CI | ✅ file, cases ❌ Wave 0 | ⬜ pending |
| D-04 / CR-02 | A non-finite `character` produces no NaN correction at the three literal-zero-width sites | unit | `make test` (`tests/test_morph_blep.cpp`) | ✅ file, cases ❌ | ⬜ pending |
| D-04 / new | A non-finite `jump` is rejected by `addStep` and leaves per-instance state untouched; a poisoned-instance trace shows recovery after the hostile input is withdrawn | unit | `make test` (`tests/test_morph_blep.cpp`) | ✅ file, cases ❌ | ⬜ pending |
| SYNC-01 | A master rising edge resets the phase; the trigger is per-instance; `syncConnected == false` never resets | unit | `make test` (`tests/test_vco_core.cpp` via `VcoBlockDriver`) | ✅ file, cases ❌ | ⬜ pending |
| SYNC-01 / D-09 | At most one rising edge is observed per sample **by construction**; every observable edge fires exactly once; the missed-edge rule is identical at 44.1 / 48 / 96 kHz; output stays finite and bounded throughout | unit | `make test` | ❌ Wave 0 | ⬜ pending |
| SYNC-02 / D-06 | The placement candidates are measured on the sync sub-grid and the winner is pinned by the three-condition decision rule (sign consistency · margin above the reproduction bound · rate signature) | spectral | `make test` (`tests/test_vco_spectrum.cpp` sync sub-grid) | ✅ apparatus, sub-grid ❌ | ⬜ pending |
| SYNC-02 / D-01 | The fractional-overshoot reset beats a snap to `phase = 0` by a measured margin on an informative master (prototype: 4.5–5.0 dB) | spectral | `make test` | ❌ Wave 0 | ⬜ pending |
| SYNC-02 / D-01 | `phase` is **never** exactly 0 after a reset — including when `f` would compute to exactly `1.0` from a master sample landing on `1.0 V` | unit | `make test` | ❌ Wave 0 | ⬜ pending |
| SYNC-02 / D-11 | Per-cell sync alias thresholds at **44.1 kHz binding**, with 48 and 96 kHz as regression; the master's bin error asserted `== 0.0` directly rather than via `impliedLeakageDb`'s `−999.0` sentinel | spectral | `make test` | ❌ Wave 0 | ⬜ pending |
| SYNC-02 / D-07 | A free-run site the reset **jumped over** is suppressed for that sample; the rule is recomputed per sample and never cached | unit | `make test` | ❌ Wave 0 | ⬜ pending |
| SC-3 / D-10 | Worst `\|x[n] − x[n−1]\|` on reset samples stays under a measured, outward-rounded bound | unit (time domain) | `make test` | ❌ Wave 0 | ⬜ pending |
| SC-3 / D-10 | **Anti-circularity:** `uncorrectedResetDelta − correctedResetDelta >= margin` — a comparison of two measurements that consults no pinned number | unit (time domain) | `make test` | ❌ Wave 0 | ⬜ pending |
| SC-3 / D-10 | A **discriminating** mutation probe fails a *stated* population exactly, not merely "some assertions" | unit | `make test` | ❌ Wave 0 | ⬜ pending |
| PITCH-04 / D-12 | Extreme pitch × extreme FM × hostile sync, re-ticked **only where sync is observed FIRING** behind the claim | unit | `make test` (`tests/test_vco_pitch.cpp`) | ✅ file, sync leg ❌ | ⬜ pending |
| D-12 / new divisor | Equal consecutive samples and a NaN cable voltage cannot poison `phase`; the recovered instance returns finite samples. Guards use the negated-comparison idiom, never `forge::clamp` | unit | `make test` (`tests/test_vco_core.cpp` scenario four grid + sync voltages) | ✅ grid exists, sync rows ❌ | ⬜ pending |
| D-02 / register item 15 | Both new `VcoInputs` fields are fed **runtime-derived** values by the canary, and `syncConnected` **varies and is true on some iterations** — otherwise the whole sync branch folds away at `-O3` while `[2b/5]` still reports PASS | build gate | `make guards` (`tests/check_canary.sh [2b/5]`) | ✅ gate exists, feeds ❌ | ⬜ pending |
| CORE-03 | The sync trigger and previous-voltage store are **per-instance**, never static; two interleaved cores do not interact across a sync window | unit | `make test` (`tests/test_vco_core.cpp` interleave invariant) | ✅ pattern exists, sync window ❌ | ⬜ pending |
| Output bound | `test_vco_core.cpp:511`'s two measured tiers (`kHostileBoundV` 10.0 V, `kMusicalBoundV` 5.55 V) **re-derived for sync, not assumed** — the prototype already reaches ±5.2 V | unit | `make test` | ✅ file, sync derivation ❌ | ⬜ pending |
| Guardrail | No frozen header edited (`Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp`); `src/AnalogLFO.cpp` absent from this phase's diff; the six shipped-LFO goldens replay byte-identical | build gate + regression | `make guards` + `make test` (`tests/test_lfo_guardrail.cpp`) | ✅ exists | ⬜ pending |
| C++11 / ODR | New core code and the new shell wiring compile under `-std=c++11 -pedantic-errors` and **link** under MinGW | build gate | `make strict` + **CI MinGW link leg** | ✅ exists | ⬜ pending |
| D-13..D-16 | The A/B renderer produces a matched pair from the **same driver in the same pass**, on demand, uncommitted | manual (operator UAT) | `make <render-target>` then listen | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/test_morph_blep.cpp` — Task 1's three RED cases (CR-01's ASan reproduction, CR-02's
      measured non-finite count, the `jump` poisoned-instance trace) and their permanent assertions.
- [ ] `tests/test_vco_spectrum.cpp` — the sync sub-grid: an in-test master generator, `SPECTRUM_GRID`-shaped
      sync cells keyed on `K_master`, the six candidate legs plus the two mutation probes, and the
      exact-zero master bin-error assertion.
- [ ] `tests/test_vco_core.cpp` — SC-3's delta bound, the anti-circularity comparison and the mutation
      probe; D-09's structural-ceiling case; the sync rows in scenario four's `HOSTILE_TIMES` grid;
      the re-derived output tiers.
- [ ] `tests/test_vco_pitch.cpp` — PITCH-04's third input class, with sync observed firing.
- [ ] `src/vco_compile_canary.cpp` — runtime-derived, **varying** feeds for `syncVolts` and `syncConnected`.
- [ ] `tools/render_sync_ab.cpp` + Makefile target + `MAKECMDGOALS` skip-filter entry + `VCO_SIDE_ALLOW` entry.
- [ ] `tests/check_includes.sh` — `VCO_SIDE_ALLOW` entries for any new TU.
- [ ] Framework install: **none** — doctest is vendored and `make test` globs.

**Wave 0 gate (D-04, RED-first):** Task 1's three defects are reproduced before they are fixed. The
ASan run is a **scoped one-shot probe**, never wired into `make test`, `make guards` or CI — register
item 12 forbids a permanent repo-wide sanitizer gate because the shipped LFO carries shared latent UB
that is deliberately unowned.

**Wave 0 gate (D-06):** the placement measurement runs against the real `forge::VcoCore` **before any
seam code is committed as final**. A plan that writes `addStep(f, jump)` and then measures has skipped
the phase's central question.

---

## Sync Sub-Grid Construction (D-06 / D-11) — recommended parameters

Extends `tests/test_vco_spectrum.cpp` verbatim: `fftRadix2`, `aliasPeakDb`, `impliedLeakageDb`,
`driveSecondBlock`, `kSpectrumN = 4096`, the one-block warm-up discard, the four seed literals
(`0x1234 / 0x5678 / 0x9E3779B9 / 0x7F4A7C15`), `base.drift = 0.f`, and `NaiveVcoCoreMirror`.

**The one structural change:** *the fundamental bin is the **master's**, not the slave's.* Under hard
sync the slave's whole trajectory is determined by the master, so the ideal continuous-time output is
exactly periodic at the master's period. With `K_m` master cycles per 4096-sample block, every true
harmonic lands on bin `n·K_m`; everything off that lattice is alias energy. Consequences:

1. `aliasPeakDb(block, K_master)` is called unchanged, with `H = { n·K_m }`.
2. **The slave frequency is free** — it is not bin-centred and must not be solved for. Neither
   `binCentredPitchCV` nor `binCentredSampleTime` applies to the slave. This is a simplification, not
   a weakening: the leakage argument attaches to the fundamental being measured.
3. **The master's bin error is exactly zero.** Generate the master in-test with a phase increment of
   `K_m · 2⁻¹²`, exactly representable and accumulating with zero rounding error. ⚠️ Assert
   `binError == 0.0` **directly** — `impliedLeakageDb(0.0)` returns the `−999.0` sentinel through its
   `!(binError > 0.0)` branch, which is semantically right here only by accident.

| Axis | Values | Why |
|------|--------|-----|
| Rate × master cycles | 44.1 kHz `K_m = 93` (1001.4 Hz) · 48 kHz `K_m = 85` (996.1 Hz) · 96 kHz `K_m = 43` (1007.8 Hz) | All odd, so coprimality holds. Within 1.2 % of 1 kHz at all three rates so cross-rate rows compare like with like. **Record the achieved master Hz on every row** — with `N` pinned at 4096, no single master frequency is integer-cycle at all three rates below ~3.4 kHz, and that spread is a property of the instrument that must be written down, not hidden. |
| Master/slave ratio | slave ≈ **0.5×**, 1×, 2×, 3×, 4×, 6× the master | ⚠️ **The `< 1` ratio is not optional.** It is where the candidates separate by 4–7 dB; in the classic sweep region (`ratio ≥ 2`) they separate by under 1.0 dB — below register item 8's reproduction bound, so no cross-toolchain decision can be taken there. |
| Morph | the five shape centres 0.00 / 0.25 / 0.50 / 0.75 / 1.00 | matches the standing grid's third-index discipline |
| Character | 0.00 and 1.00 | D-11 as written |
| Master edge shape | **at least two**: a hard-jump saw, and a 2-point-polyBLEP'd saw | ⚠️ a sub-grid built only on hard-jump masters holds `f` near-constant (`f ≈ 0.6 − g·dt_m`), so **SYNC-02's sub-sample clause goes untested** |

**Master waveform:** a ±5 V *falling* saw, `v = 5·(1 − 2·φ_m)`, whose wrap is a **rising** jump — the
Forge saw's own polarity. The falling ramp re-arms the trigger by crossing `0.1 V` downward mid-cycle
and fires it at the wrap.

**Candidate legs — six, all from the same driver in the same pass:**

| leg | what it does |
|-----|--------------|
| `none` | reset applied, sync BLEP withheld. **This is also D-14's second audition leg** — one leg serving both purposes is what "pays for itself twice" buys. |
| `a` | `addStep(f, jump)` at the detection sample (zero header change, one sample late) |
| `b` | past-edge: current sample `+= −f²/2·jump`, nothing pending |
| `c` | `addStep(0.f, jump)` (a flat half-jump on the detection sample) |
| `oracle-b` | leg `b` with `f` replaced by the master's **true** wrap fraction, which the generator knows exactly |
| `snap` | leg `b` with the reset snapped to `phase = 0` — the `STACK.md:149` landmine rendered as a measurement |

Two further legs are cheap and belong as **mutation probes**, not candidates: `mis` = `addStep(1−f, jump)`
(the natural mis-mapping of "the edge is `1−f` samples behind"), and `badsign` = leg `b` with `jump`
computed as `before − after`.

---

## Threshold Policy (D-09 / D-11, inherited from Phase 32)

Thresholds are **per cell, pinned from this phase's own measurements**, each carrying its measured
justification in the test. `kThresholdFloorDb = −75 dB` still bounds how tight any threshold can be.

**Register item 8 binds.** Every absolute decibel figure this milestone has recorded is an
**Apple-clang** figure. A cell that is **plateau**-class (no true value step, so the arg-max over
non-harmonic bins is decided by near-tied bins and one libm ULP reorders them) inherits the **4.0 dB**
reproduction bound rather than the **1.0 dB** one — and **the classification must be stated on the
physical criterion before the population is enumerated**, never as a rename of the cells that failed.

**The gate shape that will NOT work, and why it matters.** The prototype measures the sync BLEP's own
spectral improvement at a mean of ≈ **0.5 dB**. A gate written in Phase 32's shape —
`naiveDb − correctedDb >= 8.0` — therefore **fails**, and the failure is a property of the instrument,
not of the implementation. What the spectral grid *can* evidence is the **snap-to-zero landmine at
4.5–5.0 dB**, which makes the *sub-sample* half of SYNC-02 the half this instrument carries. The sync
BLEP's own non-circular evidence lives in D-10's time-domain instrument, because register item 5
measured that single-sample full-amplitude spikes read **0.0 dB spectrally** — the alias-floor gate is
structurally blind to the click SC-3 exists to forbid.

**D-06 decision rule — three conditions, all required**, evaluated at 44.1 kHz binding:

1. **Sign consistency** — the winner is ≤ every other candidate on ≥ 90 % of 44.1 kHz cells and is
   never worse than the runner-up by more than 1.0 dB on any cell.
2. **Margin above the reproduction bound** — the winner's margin over the runner-up exceeds 1.0 dB
   (step-dominated) / 4.0 dB (plateau) on at least the `ratio < 1` cells. **A decision taken only on
   cells separated by under 1.0 dB is not defensible cross-toolchain and must not be taken.**
3. **Rate signature** — the winner's margin **shrinks** from 44.1 → 48 → 96 kHz on the `ratio ≥ 1`
   cells. That shrink is the signature of a one-sample *placement* error; a margin flat across rates
   means the legs differ in jump **magnitude**, not in placement, and the measurement is measuring
   something else.

**Prototype prediction (falsifiable, not a threshold):** candidate **(b)** wins in all 54 cells;
candidate (a) is worse than applying no sync BLEP at all in 50 of 54, by up to +4.2 dB; the a-vs-b
penalty is 0.90 / 0.78 / 0.33 dB at 44.1 / 48 / 96 kHz. The prototype slave carries **one** site, not
nine — the **ranking** is expected to transfer, the **decibels must not**.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Sync reads **buzzy, not smeared** | SYNC-02 | Purely perceptual — no automated instrument exists for timbre character | Run the D-15 `make` render target, listen to the band-limited leg against the BLEP-withheld leg from the same pass, and judge whether the shipped leg is sharp rather than smeared |
| **No click per sync** at the reset | SYNC-02 / SC-3 | Perceptual — D-10 bounds the per-sample step but not audibility | Same rendered pair; listen specifically at reset instants across the grid points |
| In-Rack sync audition | SYNC-01 / SYNC-02 | The module must be patched and played | Whole-tree `rsync -a dist/ForgeAudio-AnalogSeries/` install flush (**never** copying `plugin.dylib` and `res/` alone), then patch a master oscillator into the SYNC jack of **the Analog VCO under Forge Audio Analog Series** — the operator's Rack tree carries a second, differently-slugged `ForgeAudio` plugin (register item 25) |

**The four D-17 gate precedents are binding on the UAT plan:**

1. A **blocking `.continue-here.md`** written *before* the UAT plan — a checkpoint-pending `SUMMARY.md`
   otherwise makes a resumed session walk straight past the operator gate.
2. The **full expected-results block is presented before the operator replies**, so an absence of
   complaint is an absence of complaint rather than an absence of exposure.
3. Perceptual coverage the script cannot evidence is **REFUSED, not booked**. D-13..D-16 exist so that
   this time the script *can* evidence it.
4. The session protocol names the plugin **directory** as well as the module.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] Task 1's three defects are observed RED before they are fixed (D-04), with ASan as a **scoped one-shot probe** only
- [ ] The D-06 placement measurement runs against the real core before the seam convention is finalized
- [ ] The master's bin error is asserted `== 0.0` directly, not via the `−999.0` sentinel
- [ ] The sync sub-grid includes at least one `ratio < 1` cell and at least two master edge shapes
- [ ] SC-3's anti-circularity assertion consults no pinned number
- [ ] The mutation probe fails a **stated** population exactly
- [ ] No frozen header edited; `src/AnalogLFO.cpp` absent from the diff; LFO goldens byte-identical
- [ ] CI MinGW **link** leg observed green on the exact commit
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
