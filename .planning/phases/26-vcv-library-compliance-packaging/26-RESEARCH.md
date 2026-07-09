# Phase 26: VCV Library Compliance + Packaging - Research

**Researched:** 2026-07-09
**Domain:** VCV Rack 2 plugin manifest compliance, `.vcvplugin` packaging, cross-platform C++17 CI
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** `authorUrl`, `pluginUrl`, `sourceUrl` all point to `https://github.com/Photep/ForgeAudio-AnalogSeries`. VCV permits all three at the same URL.
- **D-02:** `minRackVersion` lowered `2.6.0` → `2.0.0`. Execution must confirm with a one-time grep that no 2.1–2.6-only API is relied on before finalizing.
- **D-03:** `version` stays `2.0.0` (VCV rule: MAJOR = Rack major = 2; NOT "fixed" to the 1.4.x milestone number). Makefile `VERSION` must match `plugin.json`.
- **D-04:** Plugin slug `ForgeAudio-AnalogSeries` and module slug `ForgeAnalogLFO` are final/permanent. Run a "Forge" brand/slug collision check; flag only if a real conflict is found.
- **D-05:** CI stays the Rack-free doctest suite on ubuntu/macos/windows (fetches no Rack SDK). TEST-06 = that matrix green on push. `make dist` (PKG-05) is verified locally, not in CI.
- **D-06:** Windows `M_PI` undeclared → introduce a shared rack-free `forge::kPi` constexpr and replace every `M_PI` use. `constexpr double kPi = 3.14159265358979323846` is bit-for-bit IEEE-754 identical to `M_PI` (`0x400921FB54442D18`) so golden fixtures are NOT perturbed. (Rejected: `-D_USE_MATH_DEFINES` in test flags.)
- **D-07:** Linux golden drift exceeds 1e-5 because `std::normal_distribution` is not portable → restructure golden: cross-platform leg replays drift-OFF (deterministic, small libm epsilon); drift-ON bit-exact golden runs macOS-only. Requires generating drift-off golden fixtures. (Rejected: widening epsilon; rejected: portable hand-rolled Gaussian — changes shipped audio, violates freeze.)

### Claude's Discretion
- Exact filename/location of the `forge::kPi` header (e.g., `src/dsp/MathConst.hpp`), the precise drift-off libm epsilon value, and the drift-off golden fixture generation procedure.
- Order of operations within the phase (manifest edits vs CI fixes vs dist verification).

### Deferred Ideas (OUT OF SCOPE)
- Portable Gaussian rewrite (Box–Muller replacing `std::normal_distribution`) — changes shipped drift audio, violates v1.4 feature-freeze.
- `pluginUrl` → Notion manual swap once Phase 27 publishes it.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PKG-02 | `authorUrl`, `pluginUrl`, `sourceUrl` populated (GitHub project page) | All three are **optional** manifest fields; VCV permits identical URLs. Set to the repo per D-01. |
| PKG-03 | Manifest validated: slug permanent, version `2.0.0`, Makefile VERSION matches, tags valid, module slug/name correct, `minRackVersion` documented, no trademarked strings | Both tags confirmed on the official whitelist; version MAJOR=2 rule confirmed; **Makefile VERSION is auto-derived from plugin.json via `jq`, no manual sync needed** (see finding below); slug charset/immutability rules confirmed. |
| PKG-05 | `make dist` produces a verified `.vcvplugin` (binary + plugin.json + res/ + LICENSE, no trial fonts) | `.vcvplugin` = zstd-compressed tar (read from local plugin.mk); exact layout + verification procedure below; res/ contains only one OFL-licensed font. |
| TEST-06 | Full test suite green as a GitHub Actions CI check on push | Root causes D-06 (M_PI) + D-07 (normal_distribution) confirmed against source; **critical spread-path landmine surfaced below** that the D-07 fix must also neutralize. |
</phase_requirements>

## Summary

This is a **packaging / compliance / test-infrastructure** phase with zero DSP-behavior or shipped-audio changes. The external, verifiable facts the planner needs fall into three buckets: (1) VCV Rack 2 manifest schema — all locked decisions check out against the official manual, both existing tags are on the whitelist, and the version/slug rules are exactly as CONTEXT states; (2) `.vcvplugin` packaging — the local `../Rack-SDK/plugin.mk` was read directly, so the archive format (zstd-compressed tar, not zip), internal layout, and filename scheme are known with certainty, and all required tooling (`jq`, `zstd`, `tar`) is present on this machine; (3) the CI fixes — both root causes are confirmed in source, and I surfaced **one non-obvious landmine that will make the D-07 fix fail if missed**.

**The landmine (highest-value finding):** Turning drift OFF is *not sufficient* to make the golden portable. `BlockDriver`'s constructor unconditionally calls `core.setSpreadSeed(...)`, which draws from `std::normal_distribution` inside `DriftEngine::setSpreadSeed()`. Those component-spread coefficients (`sawCurvatureSpread`, `characterSpread`, etc.) feed the Waveshaper and perturb the output waveform *even with drift disabled*. So a drift-off replay through the current `BlockDriver` still diverges cross-platform. The drift-off fixture generation AND the cross-platform replay must **also neutralize the spread path** (no `setSpreadSeed` call → all `*Spread` coefficients stay at their `0.f` defaults). This is the only path to a bit-exact cross-platform drift-off golden.

**A second useful finding:** There is **no manual Makefile/manifest VERSION sync to perform**. `plugin.mk:6` does `VERSION := $(shell jq -r .version plugin.json)`, so VERSION is *derived* from the manifest. D-03's "Makefile VERSION must match" is automatically satisfied — the plan should *verify* this rather than *edit* a Makefile VERSION line (there is none).

**Primary recommendation:** Sequence the phase as (1) manifest edits + validation (cheap, no build), (2) the `forge::kPi` refactor (D-06) verified green on all three CI legs, (3) the golden restructure (D-07) — generate drift-off fixtures with spread *and* drift neutralized on macOS, wire the cross-platform leg to them, keep drift-on bit-exact macOS-only, (4) local `make dist` + zstd-tar unpack verification (PKG-05). Steps 2 and 3 are the only ones that can turn CI green; do them as explicit red→green commits.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Manifest field validation (PKG-02/03) | Build/Packaging metadata | — | `plugin.json` is static config consumed by Rack + the VCV Library build server |
| `.vcvplugin` artifact production (PKG-05) | Build system (Rack `plugin.mk`) | Local dev machine | `make dist` is SDK-provided; no custom packaging code is authored |
| Cross-platform determinism guard (TEST-06) | Rack-free test harness (doctest) | CI (GitHub Actions) | DSP core is plain C++17; the guard lives in `tests/`, CI just builds+runs it |
| `M_PI` portability (D-06) | DSP source (`src/dsp/*.hpp`, `AnalogLFO.cpp`) | Test TU | A shared constexpr in the DSP headers; consumed by both plugin and test builds |
| Golden fixture generation (D-07) | One-shot generator tool (to be authored) | Canonical OS = macOS | Fixtures are captured off the canonical libc++ build, committed to git |

## Standard Stack

This phase installs **no external packages**. It uses tools already present and the Rack SDK build system. The "stack" is the toolchain required by `make dist` and the CI test build.

### Core
| Tool | Version (verified local) | Purpose | Why Standard |
|------|--------------------------|---------|--------------|
| Rack `plugin.mk` / `arch.mk` | `../Rack-SDK` (present) | Defines `dist`, `install`, `all`; derives SLUG/VERSION from plugin.json | The only supported way to build a `.vcvplugin` [VERIFIED: read `../Rack-SDK/plugin.mk`] |
| `jq` | 1.8.1 | `plugin.mk` reads `.slug`/`.version` from plugin.json | Hard dependency of `make dist` — `plugin.mk:5-6` [VERIFIED: local] |
| `zstd` | 1.5.7 | Compresses the dist tar into `.vcvplugin` | `plugin.mk:95` pipes `tar … | zstd` [VERIFIED: local] |
| `tar` (bsdtar/libarchive) | 3.5.3 | Archives the `dist/<slug>/` tree | `plugin.mk:95` [VERIFIED: local] |
| doctest (bundled `tests/doctest.h`) | in-repo | Rack-free unit/golden harness | Phase 22 D-09 test discipline [VERIFIED: `tests/test_golden.cpp`] |
| clang++ / g++ (C++17) | Apple clang 16.0.0 (local); MinGW g++ (Windows CI) | Compiles plugin + test target | Rack 2 requires C++17 [VERIFIED: local + `.github/workflows/test.yml`] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| SDK `make dist` | Hand-rolled zip packaging script | Wrong format — Rack 2 `.vcvplugin` is zstd-tar, not zip; hand-rolling risks a rejected artifact. Don't. |
| `-D_USE_MATH_DEFINES` (D-06 rejected alt) | `forge::kPi` constexpr (chosen) | User rejected the macro; constexpr is include-order-independent and removes the non-standard macro entirely. |

**Installation:** None. Confirm the four dist tools are present (all verified on this machine):
```bash
jq --version && zstd --version && tar --version && command -v make
```

**Version verification:** No packages to verify against a registry — all tooling is local/SDK-provided.

## Package Legitimacy Audit

**Not applicable.** This phase installs no external packages (no npm/PyPI/crates). All tooling is either OS-provided (`jq`, `zstd`, `tar`, `make`, a C++ compiler) or vendored in-repo (`tests/doctest.h`) / SDK-provided (`../Rack-SDK`). No slopcheck run required.

## Architecture Patterns

### System Data Flow (this phase's two workstreams)

```
                          ┌─────────────────────────────────────────┐
                          │  WORKSTREAM A — Manifest & Packaging     │
                          └─────────────────────────────────────────┘
  edit plugin.json ──► jq reads .slug/.version ──► make dist ──► dist/<slug>/{binary, plugin.json,
  (URLs, minRackVer)        (plugin.mk:5-6)          │            res/, LICENSE, NOTICES}
                                                     ▼
                                          tar --no-xattrs | zstd ──► <slug>-<version>-<arch>.vcvplugin
                                                     │
                                          unzstd + tar -x ──► ASSERT: binary present, plugin.json URLs
                                                               populated, res/ present, LICENSE present,
                                                               NO trial fonts  ── PKG-05 PASS

                          ┌─────────────────────────────────────────┐
                          │  WORKSTREAM B — CI green (TEST-06)       │
                          └─────────────────────────────────────────┘
  D-06: replace M_PI  ──► forge::kPi constexpr in src/dsp/ ──► Windows g++ leg compiles ──┐
        (5 files)                                                                          │
                                                                                           ▼
  D-07: generate drift-off + spread-off fixtures on macOS ──► commit freerun_*_driftoff.f32 ──►
        cross-platform leg replays drift-OFF (small libm epsilon)  ─┐
        drift-ON bit-exact replay gated #if defined(__APPLE__)      │──► 3-OS matrix GREEN ── TEST-06 PASS
```

### Recommended Structure (delta only — extend, don't recreate)
```
src/dsp/MathConst.hpp        # NEW (Claude's discretion): forge::kPi constexpr
tests/golden/
  freerun_44100.f32          # EXISTING drift-on golden (macOS-canonical, keep)
  freerun_48000.f32
  freerun_96000.f32
  freerun_44100_driftoff.f32 # NEW: drift-OFF + spread-OFF, cross-platform guard
  freerun_48000_driftoff.f32
  freerun_96000_driftoff.f32
  freerun_seeds.txt          # UPDATE: document the drift-off scenario + spread-off note
tools/ or a one-shot TU      # NEW generator (the old capture tool is NOT in the repo)
```

### Pattern: Bit-exact vs epsilon comparator (already established, keep)
`tests/test_golden.cpp` already distinguishes `CANONICAL_OS` (bit-exact `==`) from non-canonical (absolute-epsilon). The D-07 restructure keeps this split but changes *what runs where*: the cross-platform (non-canonical) path should replay the **drift-off + spread-off** fixture with a small libm epsilon (~1e-6 for sin/cos variance), and the **drift-on** replay should be gated to `#if defined(__APPLE__)` only, not merely given a larger epsilon.

### Anti-Patterns to Avoid
- **Widening the drift-on epsilon to absorb `normal_distribution` divergence** — CONTEXT D-07 forbids this; the divergence is on the order of drift depth, so a covering epsilon stops catching real regressions.
- **Editing a Makefile `VERSION` line** — there is none; VERSION is `jq`-derived from plugin.json. Verify, don't edit.
- **Assuming drift=0 alone makes the replay portable** — see the spread-path landmine (Pitfall 1). It does not.
- **Hand-authoring a `dist`/zip target** — `make dist` already exists and produces the correct zstd-tar.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| `.vcvplugin` packaging | Custom zip/tar script | `make dist` (plugin.mk) | Rack 2 format is zstd-tar with a specific `<slug>/` root + filename scheme; SDK owns it |
| Manifest VERSION sync | A Makefile VERSION var + sync check | `jq -r .version plugin.json` (already in plugin.mk) | Single source of truth already wired |
| Portable π | A runtime `4*atan(1)` or macro juggling | `constexpr double kPi = 3.14159265358979323846` | Compile-time, bit-identical to M_PI, no build-flag reliance (D-06) |

**Key insight:** Nearly every "build a thing" impulse in this phase is already solved by the Rack SDK or existing repo infrastructure. The genuine authoring work is small: one constexpr header, one fixture generator, and the golden-test restructure.

## Runtime State Inventory

> This phase is a compliance/refactor phase (M_PI rename + golden restructure), so a runtime-state audit applies to the refactor scope.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | **None** — the only persisted artifacts are `tests/golden/*.f32` (git-tracked); no external datastore keys reference `M_PI` or drift naming. | Add drift-off `.f32` fixtures (new committed data) |
| Live service config | **None** — no external service embeds phase strings; CI config lives in `.github/workflows/test.yml` (git-tracked). | Update the Windows CI `g++` line only if the kPi refactor needs no flag (it does not) |
| OS-registered state | **None** — no scheduled tasks, daemons, or OS registrations. | None |
| Secrets/env vars | **None** — no secrets; CI needs no tokens for the Rack-free build. | None |
| Build artifacts | `build-test/` (test binary), `dist/` (from `make dist`) — both regenerated, gitignored. `M_PI` appears in 5 source files (22 occurrences): `src/AnalogLFO.cpp`, `src/dsp/DriftEngine.hpp`, `src/dsp/Waveshape.hpp`, `src/dsp/LfoCore.hpp`, `tests/test_extraction.cpp`. | Replace all 22 M_PI with `forge::kPi`; rebuild test + plugin |

**Canonical question — after every file is updated, what still holds the old string?** Nothing outside the 5 source files and the (regenerable) golden fixtures. The refactor is fully contained in the tree. [VERIFIED: `grep -rn M_PI src/ tests/`]

## Common Pitfalls

### Pitfall 1: The spread path also uses `std::normal_distribution` — drift-off alone is NOT portable  *(CRITICAL)*
**What goes wrong:** The D-07 plan generates a drift-off golden expecting cross-platform bit-exactness, but the cross-platform leg still diverges (or needs an implausibly large epsilon), leaving Linux/Windows red.
**Why it happens:** `tests/BlockDriver.hpp` constructor **always** calls `core.setSpreadSeed(sp0, sp1)`. That routes into `DriftEngine::setSpreadSeed()` (`src/dsp/DriftEngine.hpp:98`), which constructs `std::normal_distribution<float> d{0,1}` and draws `characterSpread`, `sawCurvatureSpread`, `squareDutySpread`, `triAsymmetrySpread`, `bleedSpread`, `ouWeightSpread[]`. `LfoCore::setSpreadSeed` (`src/dsp/LfoCore.hpp:101-109`) forwards these into the Waveshaper, so they perturb the *waveform* independent of drift. `std::normal_distribution` is non-portable across libc++/libstdc++/MinGW → the drift-off output still differs cross-platform.
**How to avoid:** For the cross-platform drift-off fixture, **neutralize the spread path too** — do not call `setSpreadSeed` (leave all `*Spread` at their `0.f` defaults). Options for the planner/executor: add a `BlockDriver` construction mode that skips `setSpreadSeed`, or a dedicated generator that seeds only drift and never spread. The resulting output uses only the portable Xoroshiro uniform stream + libm `sin`/`cos` → bit-exact-to-~1e-6 cross-platform.
**Warning signs:** Drift-off replay passes on macOS but fails on Linux/Windows at any epsilon below ~1e-3. [VERIFIED: read `tests/BlockDriver.hpp`, `src/dsp/DriftEngine.hpp`, `src/dsp/LfoCore.hpp`]

### Pitfall 2: `minRackVersion` is only enforced by Rack 2.4.0+
**What goes wrong:** Assuming the floor blocks older Rack; or assuming it is validated at submission.
**Why it happens:** Only VCV Rack ≥ 2.4.0 enforces `minRackVersion`; older hosts ignore it. Lowering to `2.0.0` (D-02) is safe and maximally compatible.
**How to avoid:** Keep D-02. Confirm no 2.1–2.6-only API is used. My grep found `getLastFrameDuration` (Rack 2.0), `createIndexSubmenuItem` (Rack 2.0) — both exist since 2.0. No 2.1–2.6-only symbol found. [CITED: vcvrack.com/manual/Manifest] [VERIFIED: `grep` of `src/`]

### Pitfall 3: Slug is permanent and charset-restricted
**What goes wrong:** A late slug rename breaks patch compatibility forever.
**Why it happens:** "After your plugin is released, the slug must *never* change." Slugs allow only `a-z A-Z 0-9 - _`. `ForgeAudio-AnalogSeries` and `ForgeAnalogLFO` are both valid. D-04 locks them.
**How to avoid:** Run the collision check against the live VCV Library plugin list before submission; only flag on a real conflict. [CITED: vcvrack.com/manual/Manifest]

### Pitfall 4: `.vcvplugin` is a zstd-tar, not a zip
**What goes wrong:** PKG-05 verification tries `unzip file.vcvplugin` and fails, or a reviewer expects zip layout.
**Why it happens:** Rack 1.x used zip; Rack 2.x uses `tar … | zstd` (`plugin.mk:95`). Unpack with zstd+tar, not unzip.
**How to avoid:** Verify with `zstd -d` / `tar --zstd` (procedure in Code Examples). [VERIFIED: `../Rack-SDK/plugin.mk:93-95`]

### Pitfall 5: The golden-fixture generator is not in the repo
**What goes wrong:** The plan assumes a capture tool exists to regenerate fixtures; it doesn't.
**Why it happens:** `freerun_seeds.txt` references `build-test/capture (one-shot generator, not part of make test)` but no such source file is committed (searched `src/`, `tests/`, `tools/` — none found; `tools/` does not exist).
**How to avoid:** Plan a task to author a small one-shot generator TU that constructs a `BlockDriver` (drift-off, spread-off), runs `GOLDEN_SAMPLES` at each rate, and writes little-endian f32. Run it on macOS (canonical OS) and commit the output. [VERIFIED: repo search]

### Pitfall 6: Submission is source-repo based, versioned by commit hash
**What goes wrong:** First-timer expects to upload a binary or a `.vcvplugin`.
**Why it happens:** The VCV Library builds from your **public source repository**. You open one issue in `VCVRack/library` titled exactly your **slug** (not name), post the source URL, and for updates bump `version` in plugin.json, push, and post the **commit hash** (`git rev-parse HEAD`, not a branch name). *(Submission itself is Phase 27+ scope, but the manifest must be submission-ready now.)* [CITED: github.com/VCVRack/library]

## Code Examples

### `forge::kPi` constexpr (D-06)
```cpp
// src/dsp/MathConst.hpp (name is Claude's discretion)
#pragma once
namespace forge {
// Bit-for-bit identical IEEE-754 double to cmath's pi macro (0x400921FB54442D18),
// so (float)kPi and (double)kPi equal cmath's pi value exactly. Golden fixtures unperturbed.
// NOTE: keep the literal cmath pi macro token OUT of this comment — plan 26-02's
// `grep -rn M_PI src/ tests/` zero-hit gate scans this header.
inline constexpr double kPi = 3.14159265358979323846;
}
// Replace every `(float)M_PI` -> `(float)forge::kPi`, `2.f * (float)M_PI` etc.
// Files: src/AnalogLFO.cpp, src/dsp/DriftEngine.hpp, src/dsp/Waveshape.hpp,
//        src/dsp/LfoCore.hpp, tests/test_extraction.cpp  (22 occurrences total)
```

### Drift-off + spread-off fixture generation (D-07, neutralizing Pitfall 1)
```cpp
// One-shot generator (run on macOS canonical OS; NOT part of `make test`).
// Key: do NOT call setSpreadSeed -> all *Spread stay 0.f -> no normal_distribution.
forge::LfoCore core;
core.seed(DRIFT_S0, DRIFT_S1);      // drift RNG only; NO core.setSpreadSeed(...)
forge::Inputs in = goldenBase();
in.drift = 0.0f;                    // drift OFF
// step GOLDEN_SAMPLES times at sr, write little-endian float32 to freerun_<sr>_driftoff.f32
```
```cpp
// test_golden.cpp restructure (cross-platform leg):
//   - replay freerun_<sr>_driftoff.f32 EVERYWHERE with abs epsilon ~1e-6 (libm sin/cos)
//   - keep drift-on bit-exact replay gated:  #if defined(__APPLE__)  ... #endif
```

### PKG-05: build and verify the `.vcvplugin`
```bash
# Produces dist/<slug>/... and dist/<slug>-<version>-<arch>.vcvplugin  (e.g. mac-arm64)
make dist

# Inspect layout WITHOUT installing (zstd-tar, NOT zip):
tar --zstd -tvf dist/ForgeAudio-AnalogSeries-2.0.0-mac-arm64.vcvplugin
# or:  zstd -dc dist/ForgeAudio-AnalogSeries-*.vcvplugin | tar -tvf -

# Assert contents (all under ForgeAudio-AnalogSeries/):
#   plugin.dylib (or .so/.dll)   <- the built binary
#   plugin.json                  <- URLs populated, version 2.0.0
#   res/                         <- SVGs + fonts/JetBrainsMonoNL-Regular.ttf + fonts/OFL.txt
#   LICENSE, NOTICES             <- present
#   NO trial/commercial fonts    <- res/fonts holds ONLY the OFL font + OFL.txt
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `.vcvplugin` = zip archive | `.vcvplugin` = zstd-compressed tar | Rack 2.0 | Verify with zstd+tar, not unzip |
| Tags in `Model::create(...)` C++ | Tags in `plugin.json` `.modules[].tags` | Rack 1.0+ | Manifest is the single source |
| `"LFO"` / `"Low frequency oscillator"` tag | `"Low-frequency oscillator"` (canonical) | — | Old spellings are deprecated aliases (still accepted, case-insensitive) |

**Deprecated/outdated:**
- `M_PI` reliance for portability — non-standard (POSIX/`_USE_MATH_DEFINES`), replaced here by `forge::kPi`.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The macOS `make dist` arch suffix is `mac-arm64` on this Apple-silicon machine | Code Examples / PKG-05 | Low — filename cosmetic; `arch.mk` computes `ARCH_OS-ARCH_CPU`; verify actual output name |
| A2 | `createIndexSubmenuItem` and `getLastFrameDuration` are Rack-2.0-era (support minRackVersion 2.0.0) | Pitfall 2 | Low-Med — D-02 already mandates a one-time confirming grep; if a 2.x-only symbol surfaces, raise the floor |
| A3 | A small absolute epsilon (~1e-6) covers cross-platform libm sin/cos variance for the drift-off + spread-off replay | Code Examples | Med — exact value is Claude's discretion; if too tight, tune upward but keep well below drift depth |

## Open Questions (RESOLVED)

1. **Should the drift-off fixtures reuse the same seeds/params as the drift-on golden (except drift & spread)?** — **RESOLVED**
   - RESOLVED: plan 26-03 decides identical scenario params (rate 2.0, morph 0.4, character 0.6, pinned seeds) with only `drift=0.0f` and NO `setSpreadSeed` call, keeping the two goldens comparable; the delta is documented in `freerun_seeds.txt`. (Claude's-Discretion item, decided in 26-03.)
   - What we know: `freerun_seeds.txt` documents drift-on params (rate 2.0, morph 0.4, character 0.6, drift 0.5, seeds pinned). BlockDriver defaults exist.
   - What's unclear: whether to keep character=0.6 (its spread folding is now zeroed) or document a distinct drift-off scenario.
   - Recommendation (adopted): Keep identical scenario params, only `drift=0` and no `setSpreadSeed`; document the delta in `freerun_seeds.txt`. Keeps the two goldens comparable and the intent auditable.

2. **Where to house the fixture generator so it's reproducible but excluded from `make test`?** — **RESOLVED**
   - RESOLVED: plan 26-03 houses the generator at `tools/capture_golden.cpp`, compiled by a dedicated `capture` Makefile target (Rack-free, not the `test` target). (Claude's-Discretion item, decided in 26-03.)
   - Recommendation (adopted): a `tools/` TU or a `capture`-suffixed source compiled by a dedicated Makefile target (not the `test` target). Matches the historical `build-test/capture` intent without polluting the doctest suite.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| jq | `make dist` (SLUG/VERSION) | ✓ | 1.8.1 | — (hard dep) |
| zstd | `make dist` archive | ✓ | 1.5.7 | — (hard dep) |
| tar (bsdtar) | `make dist` + verify | ✓ | 3.5.3 | GNU tar also works |
| make | build orchestration | ✓ | GNU Make 3.81 | — |
| clang++/g++ (C++17) | plugin + test build | ✓ | Apple clang 16.0.0 | — |
| `../Rack-SDK` | `make dist`/`make install` | ✓ | present | — (dist cannot run without it) |
| MinGW g++ | Windows CI test leg | (GitHub-hosted `windows-latest`) | preinstalled | — |

**Missing dependencies with no fallback:** None. All tooling for local `make dist` (PKG-05) is present on this machine.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | doctest (vendored `tests/doctest.h`), C++17 |
| Config file | none — `make test` target in `Makefile` (TEST_-namespaced, Rack-free) |
| Quick run command | `make test` |
| Full suite command | `make test` (single binary runs all TUs) |
| Windows CI invocation | direct `g++ -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off tests/*.cpp -o test.exe && ./test.exe` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TEST-06 | 3-OS matrix green | CI (unit/golden) | `make test` (unix) / direct g++ (win) | ✅ `.github/workflows/test.yml` |
| D-06 | No M_PI; kPi compiles on Windows | unit (build) | Windows g++ leg compiles clean | ✅ (leg exists; must go green) |
| D-07 | Cross-platform golden green | golden | `make test` → `test_golden.cpp` | ⚠️ needs drift-off fixtures + restructure (Wave 0) |
| PKG-05 | `.vcvplugin` layout correct | manual/script | `make dist` + `tar --zstd -tvf` assertions | ✅ tooling present; artifact generated at run |
| PKG-02/03 | Manifest valid | manual/lint | `jq . plugin.json` + field review | ✅ manifest present |

### Sampling Rate
- **Per task commit:** `make test`
- **Per wave merge:** `make test` on macOS + push to trigger the 3-OS matrix
- **Phase gate:** 3-OS matrix green AND local `make dist` verified before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `src/dsp/MathConst.hpp` (or chosen name) — `forge::kPi` (blocks D-06)
- [ ] `tests/golden/freerun_{44100,48000,96000}_driftoff.f32` — drift-off + spread-off fixtures (blocks D-07)
- [ ] Fixture generator TU/target (the old `build-test/capture` is not in the repo)
- [ ] `test_golden.cpp` restructure: cross-platform leg → drift-off fixtures; drift-on bit-exact → `#if defined(__APPLE__)`

## Security Domain

VCV Rack audio plugins have no network, auth, session, or untrusted-input surface in this phase. ASVS categories are largely N/A; the only relevant control is manifest/asset integrity.

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V5 Input Validation | marginal | `plugin.json` is static, hand-authored JSON; validate with `jq .` and field-by-field review (PKG-03) |
| V6 Cryptography | no | none |
| V2/V3/V4 (auth/session/access) | no | none |

**VCV-specific integrity requirements (Ethics Guidelines):** the plugin must not harm the user's computer or privacy, and must not clone brand/model names, logos, or panel designs without permission. Phase 25 IP work (wordmark outlines, OFL fonts, NOTICES) addresses the asset-provenance side; this phase must not reintroduce trademarked strings in the manifest (PKG-03). [CITED: vcvrack.com/manual/PluginLicensing]

## Sources

### Primary (HIGH confidence)
- `../Rack-SDK/plugin.mk` (lines 5-6, 11-15, 71-95), `../Rack-SDK/arch.mk:33`, `../Rack-SDK/compile.mk:44-47` — dist format, VERSION/SLUG derivation, Windows `-D_USE_MATH_DEFINES` injection [read directly]
- `tests/BlockDriver.hpp`, `src/dsp/DriftEngine.hpp`, `src/dsp/LfoCore.hpp`, `tests/test_golden.cpp`, `tests/golden/freerun_seeds.txt` — spread-path landmine, golden structure [read directly]
- `plugin.json`, `Makefile`, `.github/workflows/test.yml`, `NOTICES`, `res/` tree — current state [read directly]
- vcvrack.com/manual/Manifest — manifest fields, slug/version rules, tag whitelist, minRackVersion enforcement (2.4.0+)
- github.com/VCVRack/library — submission process (issue titled by slug, source URL, commit-hash updates)

### Secondary (MEDIUM confidence)
- library.vcvrack.com/?tag=Waveshaper and ?tag=Low-frequency+oscillator — confirm both tags are live/valid
- vcvrack.com/manual/PluginLicensing — GPLv3+ acceptable, Ethics Guidelines (no brand cloning)

### Tertiary (LOW confidence)
- None material — all load-bearing claims cross-checked against the local SDK or official docs.

## Metadata

**Confidence breakdown:**
- Manifest schema (PKG-02/03): HIGH — official manual + live library + local plugin.json
- Packaging / `.vcvplugin` (PKG-05): HIGH — read directly from the local Rack SDK plugin.mk; tooling verified present
- CI root causes + spread landmine (TEST-06): HIGH — confirmed against actual source files
- Fixture generation procedure: MEDIUM — approach is clear, exact epsilon and generator housing are Claude's discretion

**Research date:** 2026-07-09
**Valid until:** ~2026-08-09 (VCV manifest schema is stable; SDK build rules pinned by the local `../Rack-SDK`)
