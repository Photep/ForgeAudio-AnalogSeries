---
phase: 32
slug: morph-aware-anti-aliasing-polyblep-polyblamp
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-08-01
---

# Phase 32 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Derived from `32-RESEARCH.md` § Validation Architecture (all figures `[MEASURED]` against the frozen `Waveshape.hpp`).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest (header-only, in-tree) |
| **Config file** | `Makefile` — `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`; `TEST_SOURCES` globs `tests/*.cpp` |
| **Quick run command** | `make test` |
| **Full suite command** | `make test && make strict && make guards` |
| **Estimated runtime** | ~30 seconds (`make test`); the spectral sweep dominates |

**New wiring needed:** none for the build (the `tests/*.cpp` glob picks up new files automatically); **one line** in `tests/check_includes.sh` `VCO_SIDE_ALLOW` for the new spectral test; **one active include** in `src/vco_compile_canary.cpp`.

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test && make strict && make guards`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

`make strict` is non-optional at wave boundaries: the VCV library toolchain is `-std=c++11` GCC, and clang-masked C++17/ODR bugs fail there rather than locally.

---

## Per-Task Verification Map

Task IDs are assigned by the planner; this map is keyed by requirement until plans exist. Every row must resolve to at least one `<automated>` verify in a PLAN.md task.

| Requirement | Behavior | Threat Ref | Test Type | Automated Command | File Exists | Status |
|-------------|----------|------------|-----------|-------------------|-------------|--------|
| TEST-03 / AA-01 | Alias floor per `(morph region, note, character)` at 44.1 kHz stays below its pinned per-shape threshold | — | spectral invariant | `make test` (`tests/test_vco_spectrum.cpp`) | ❌ Wave 0 | ⬜ pending |
| TEST-03 / D-08 | Naive baseline recorded per shape and per note; the gate is provably RED before `MorphBlep` lands | — | spectral invariant | `make test` | ❌ Wave 0 | ⬜ pending |
| TEST-03 / D-10 | The gate asserts its own leakage floor is ≥ 10 dB below every threshold it asserts | — | unit self-check | `make test` | ❌ Wave 0 | ⬜ pending |
| D-11 | Same assertions hold at 48 kHz and 96 kHz on the same note (C8) | — | regression | `make test` | ❌ Wave 0 | ⬜ pending |
| AA-02 | Triangle polyBLAMP fires and improves the floor at character 0; correctly vanishes at character 1 | — | spectral invariant | `make test` (tri rows) | ❌ Wave 0 | ⬜ pending |
| AA-03 | Overlapping pulse edges sum rather than overwrite | — | unit + spectral | `make test` (pulse at C9, `duty = 0.05`) | ❌ Wave 0 | ⬜ pending |
| AA-04 | Characterized jumps: correction → 0 as the softened edge widens past 2·`dt`; one measured jump asserted per site | — | unit | `make test` — `morphBlepCharFactor` limits + per-site jump | ❌ Wave 0 | ⬜ pending |
| AA-05 | Table-free, Rack-free, C++11-strict | — | build gate | `make strict && make guards` | ✅ exists | ⬜ pending |
| CORE-02 | Frozen headers untouched; the new header sits in the seam | — | build gate | `make guards` (`check_frozen`, `check_canary`, `check_includes`) | ✅ exists | ⬜ pending |
| CORE-02 / D-14 | No static/global mutable state in `MorphBlep`; two interleaved cores do not interact | — | unit | `make test` (`test_vco_core.cpp` interleave case) | ✅ pattern exists | ⬜ pending |
| MORPH-01 / MORPH-02 | Audio-rate MORPH sweep (knob + CV + attenuverter) stays finite and bounded through segment boundaries | — | unit | `make test` | ❌ Wave 0 | ⬜ pending |
| D-15 | Hostile `sampleTime`/`sampleRate` grid extended to ±inf, subnormal, very-large-finite | — | unit | `make test` (`test_vco_core.cpp` scenario four) | ✅ file exists, grid ❌ | ⬜ pending |
| Output bound | Corrected output stays within ±5.55 V across the musical range and within ~10.0 V on the hostile-timing grid | — | unit | `make test` | ✅ file exists, bound ❌ | ⬜ pending |
| LFO guardrail | The shipped LFO golden is bit-stable and `src/AnalogLFO.cpp` is absent from this phase's diff | — | regression | `make test && make guards` | ✅ exists | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/test_vco_spectrum.cpp` — the DFT/Goertzel helper, the D-10 bin construction, the naive baseline sweep, and the per-shape threshold table (TEST-03, AA-01, AA-02, AA-03, D-08, D-09, D-10, D-11)
- [ ] `tests/check_includes.sh` — one `VCO_SIDE_ALLOW` entry for the new spectral test file
- [ ] `src/vco_compile_canary.cpp` — activate the pre-staged `MorphBlep.hpp` include

No framework install is required — doctest and the `tests/*.cpp` glob are already in place.

**Wave 0 gate (D-08):** the spectral helper and the naive baseline land *before* any band-limiting exists, and the alias-floor gate must be observed FAILING against the naive core. A gate written against already-passing code proves nothing.

---

## Spectral Construction (D-10) — pinned parameters

| Parameter | Value | Why |
|-----------|-------|-----|
| Block length `N` | 4096 | `N = 2¹²` reduces D-10's coprimality requirement to "pick an odd `K`"; 92.9 ms at 44.1 kHz |
| Cycle count `K` | odd, per rate/note | `gcd(K, N) = 1` for every odd `K`, so aliases never collide with true harmonics |
| Window | rectangular | Exact, by construction — integer cycles per block means zero leakage |
| Warm-up | one discarded block of `N` samples | The pending accumulator and phase must reach steady state |
| Alias metric | `20·log10( max_{i ∈ [1, N/2] \ H} \|X_i\| / \|X_K\| )` where `H = { nK }` | Peak is the gate; RMS reported alongside as diagnostic |
| Bin 0 | excluded | The narrow pulse legitimately carries DC; the DC blocker is Phase 34's |

**Test frequencies** (`f = K·f_s/N`, all `K` odd):

| Rate | `K` | `f` (Hz) | ≈ note | harmonics below Nyquist |
|------|-----|---------:|--------|------------------------:|
| 44100 | 97 | 1044.4 | C6 | 21 |
| 44100 | 195 | 2099.5 | **C7** | 10 |
| 44100 | 389 | 4188.2 | **C8** | 5 |
| 44100 | 777 | 8366.9 | **C9** | 2 |
| 48000 | 357 | 4183.6 | C8 | 5 |
| 96000 | 179 | 4195.3 | C8 | 11 |

The gate asserts on **C7, C8 and C9** (operator decision, 2026-08-01, superseding the undefined "top two octaves"). C6 is retained as a diagnostic row. The 48 kHz and 96 kHz entries deliberately land on the same note as the 44.1 kHz C8 row so D-11's cross-rate regression compares like with like.

**Bin-centre placement:** bisect on `pitchCV` and keep `VcoBlockDriver` unchanged (achieved error 2.3e-4 … 1.5e-3 bins → −56 … −73 dB implied leakage). The gate MUST self-check: measure the achieved `|deltaPhase − K/N|·N`, compute the implied leakage, and `REQUIRE` it sits at least 10 dB below the threshold being asserted in that case. Any case needing tighter than ≈ −50 dB (the sine rows) switches to nudging the injected `sampleTime`. Without this self-check the gate can pass by measuring `exp2_taylor5`'s output granularity — D-10's stated failure mode wearing a different costume.

---

## Threshold Policy (D-09)

Thresholds are **per `(morph region, note, character)`, pinned from this phase's own measurements** — not inherited from the roadmap. The ROADMAP's former "≈ −60 dB" figure was corrected on 2026-08-01: 2-sample polyBLEP attenuates by `sinc²`, which is only ≈ −8 dB at Nyquist, so −60 dB is unreachable by the technique, not by this implementation (corroborated by DAFx-16, which reports the same ceiling for a four-point polyBLAMP).

Each threshold carries its measured justification in the test. The prototype figures below are the **expected** result; D-08's job is to reproduce them against the real `MorphBlep` and pin from *those* numbers.

Peak alias, dB rel. fundamental, 44.1 kHz, spreads = 0, **naive → corrected**:

| character | note | sine | triangle | saw | square | pulse (5%) |
|-----------|------|------|----------|-----|--------|------------|
| **0.00** | C7 | −150.7 → −150.7 | −41.7 → **−50.3** | −20.8 → **−29.5** | −20.8 → **−29.5** | −4.8 → **−13.5** |
| | C8 | −150.7 → −150.7 | −33.8 → **−48.8** | −15.6 → **−25.8** | −16.9 → **−31.9** | −1.3 → **−11.6** |
| | C9 | −150.7 → −150.7 | −19.1 → **−28.5** | −9.5 → **−19.0** | −9.5 → **−19.0** | −0.3 → **−9.8** |
| **0.50** | C7 | −60.0 → −68.4 | −40.7 → −42.1 | −20.6 → −29.2 | −22.0 → −30.3 | −5.3 → −13.1 |
| | C8 | −55.5 → −71.5 | −33.2 → −38.1 | −15.4 → −25.7 | −17.5 → −33.2 | −1.5 → −11.1 |
| | C9 | −36.1 → −34.6 | −19.0 → −24.9 | −9.4 → −18.9 | −9.6 → −19.6 | −0.4 → −9.5 |
| **1.00** | C7 | −102.4 → −102.4 | −47.4 → −47.4 | −20.0 → **−28.0** | −53.0 → −60.1 | −19.7 → −20.3 |
| | C8 | −70.6 → −76.4 | −33.5 → −33.5 | −14.7 → **−23.9** | −40.1 → −47.7 | −7.6 → −10.8 |
| | C9 | −23.2 → −22.7 | −18.5 → −19.8 | −8.9 → −17.5 | −15.5 → −21.7 | −2.3 → −5.6 |

**Three readings the plans must honor:**
1. The **saw** is the only shape whose correction is character-independent (~9 dB at every character).
2. The **triangle's** correction correctly vanishes at character 1 (`k → 0`; the corner is already 7.7 samples wide). A single "triangle at C8" number is either vacuously passed by the naive path or wrongly failed by the correct implementation.
3. The **sine row is entirely the bleed ring** — a sine has no discontinuity of its own, yet reaches −23 dB at character 1 / C9. D-04's "the trap this phase would most plausibly have missed" is confirmed and quantified.

**Recommended threshold form:** the `(region, note, character)` matrix **plus** a no-regression invariant (corrected must never be worse than naive), so a future refactor cannot quietly trade one shape against another.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Audio-rate MORPH sweep sounds clean through segment boundaries | MORPH-01 / MORPH-02 | Perceptual — the spectral gate bounds alias energy but not audible artefacts at crossfade seams | Build and install the plugin, patch an audio-rate LFO into MORPH CV with the attenuverter at full, sweep the MORPH knob across all five shapes at C7–C9, and listen for zipper noise or discontinuities at the segment boundaries |

All other phase behaviors have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] The alias-floor gate is observed RED against the naive core before `MorphBlep` lands (D-08)
- [ ] The gate asserts its own leakage floor (D-10 self-check)
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
