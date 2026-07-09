# Phase 26: VCV Library Compliance + Packaging - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-09
**Phase:** 26-vcv-library-compliance-packaging
**Areas discussed:** Manifest URLs, minRackVersion, Slug/brand lock, CI scope, CI cross-platform failures (M_PI, golden non-portability)

---

## Manifest URLs (PKG-02)

| Option | Description | Selected |
|--------|-------------|----------|
| All three → GitHub repo | authorUrl/pluginUrl/sourceUrl all point to the repo; simplest, always reachable | ✓ |
| GitHub now, swap pluginUrl later | All three → repo now, follow-up to point pluginUrl at the Notion manual after Phase 27 | |
| I have a brand/site URL | Use a Forge Audio website for authorUrl/pluginUrl | |

**User's choice:** All three → GitHub repo (`https://github.com/Photep/ForgeAudio-AnalogSeries`).
**Notes:** User first asked for an explanation of why the choice mattered. Clarified the three fields' roles (source code link, author page, plugin website), that they must be non-empty/reachable, and that VCV permits all three being identical. User then chose the repo for all three.

---

## minRackVersion

| Option | Description | Selected |
|--------|-------------|----------|
| Lower to 2.0.0 | Widest Rack 2.x compatibility; no 2.6 API used | ✓ |
| Keep 2.6.0 | Only Rack 2.6+ users; no technical requirement forcing it | |
| Pick a middle version | A specific floor between 2.0 and 2.6 | |

**User's choice:** Lower to 2.0.0.
**Notes:** Grep of `src/` found no Rack 2.6-specific API. Execution to confirm the floor with a one-time API grep.

---

## Slug / brand lock (PKG-03)

| Option | Description | Selected |
|--------|-------------|----------|
| Keep slugs, verify no collision | Treat slugs as final; run a VCV Library "Forge" collision check, flag only real conflicts | ✓ |
| Keep slugs, skip the check | Lock slugs without a formal check | |
| I'm reconsidering the name | Revisit the brand/module name before locking | |

**User's choice:** Keep slugs, verify no collision.
**Notes:** Plugin slug `ForgeAudio-AnalogSeries`, module slug `ForgeAnalogLFO` — permanent once published.

---

## CI scope (TEST-06)

| Option | Description | Selected |
|--------|-------------|----------|
| Keep Rack-free test suite only | TEST-06 = existing doctest suite green on 3 OSes; matches D-09 | ✓ |
| Add a plugin-build CI job | Also compile the real plugin against the Rack SDK in CI | |
| Just verify current CI is green | No CI changes, only confirm the workflow passes | |

**User's choice:** Keep Rack-free test suite only.
**Notes:** During the "ready for context?" gate, the user surfaced that CI is actually **red** on Windows + Linux and requested a full diagnosis — reframing TEST-06 as unmet. This became a fifth discussion area (below).

---

## CI cross-platform failures — Failure 1: M_PI (Windows test build)

| Option | Description | Selected |
|--------|-------------|----------|
| forge::kPi constexpr | Shared rack-free constexpr replacing all M_PI; no macro, include-order-proof, bit-identical numerics | ✓ |
| -D_USE_MATH_DEFINES in test flags | Add the define to TEST_CXXFLAGS + CI Windows g++ line, mirroring Rack's compile.mk | |

**User's choice:** forge::kPi constexpr.
**Notes:** Diagnosis confirmed the failure is **tests-only** — `../Rack-SDK/compile.mk:45` injects `-D_USE_MATH_DEFINES` for the shipped Windows plugin build, so the plugin is unaffected. User's hypothesis about the mechanism was correct; the "hits the real plugin build" part was disproven. kPi value chosen to keep golden fixtures bit-exact.

---

## CI cross-platform failures — Failure 2: golden non-portability (Linux)

| Option | Description | Selected |
|--------|-------------|----------|
| Restructure golden (drift-off cross-platform + drift-on macOS-only) | Real cross-platform guard on deterministic path; drift-on bit-exact only on canonical OS; needs drift-off fixtures | ✓ |
| Skip drift-on off-canonical | Mark drift-on golden informational on Linux/Windows; rely on test_invariants.cpp; weaker guard | |
| Decide during planning | Defer only the golden strategy to the planner | |

**User's choice:** Restructure golden (drift-off cross-platform + drift-on macOS-only).
**Notes:** Root cause = `std::normal_distribution` non-portability (`DriftEngine.hpp:46,98`), an algorithmic divergence, not FP codegen noise or a DSP regression. Widening the 1e-5 epsilon rejected (would gut the regression guard). Portable-Gaussian rewrite rejected as out-of-scope (changes shipped audio, violates feature-freeze) → deferred. Test-only fix.

---

## Claude's Discretion

- Filename/location of the `forge::kPi` header (e.g. `src/dsp/MathConst.hpp`).
- Precise drift-off libm epsilon (~1e-6) and the drift-off golden fixture generation procedure.
- Order of operations within the phase.

## Deferred Ideas

- Portable Gaussian rewrite (bit-exact drift on all platforms) — changes shipped audio; future DSP milestone.
- Swap `pluginUrl` → Notion manual after Phase 27 publishes it.
