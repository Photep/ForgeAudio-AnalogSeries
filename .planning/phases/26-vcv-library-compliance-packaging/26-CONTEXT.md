# Phase 26: VCV Library Compliance + Packaging - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Take the feature-frozen Analog LFO to a submission-ready state for the VCV Library:
validate `plugin.json` field-by-field, populate the manifest URLs, produce and verify
a `.vcvplugin` artifact via `make dist`, and get the full test suite **green in CI on
all three OS legs** (currently red on Windows + Linux).

**Feature-frozen:** No DSP behavior changes. No shipped-audio changes. This is a
packaging / compliance / test-infrastructure phase only. Any fix that would alter the
plugin's audio output (e.g., changing the drift RNG) is explicitly out of scope.

Requirements owned here: **PKG-02, PKG-03, PKG-05, TEST-06** (per ROADMAP.md).

</domain>

<decisions>
## Implementation Decisions

### Manifest URLs (PKG-02)
- **D-01:** `authorUrl`, `pluginUrl`, and `sourceUrl` all point to the GitHub repo:
  `https://github.com/Photep/ForgeAudio-AnalogSeries`. This satisfies PKG-02 immediately,
  is guaranteed reachable, and does not block on the Phase 27 Notion manual. VCV permits
  all three pointing at the same URL. (Optional future follow-up, NOT part of this phase:
  swap `pluginUrl` to the Notion manual once Phase 27 publishes it.)

### Manifest version + minRackVersion (PKG-03)
- **D-02:** `minRackVersion` lowered `2.6.0` → `2.0.0` for widest Rack 2.x compatibility.
  No Rack 2.6-specific API is used (`getLastFrameDuration` and the menu/component helpers
  all exist since Rack 2.0). **Execution must confirm** with a one-time grep that no
  2.1–2.6-only API is relied on before finalizing the floor.
- **D-03:** `version` stays `2.0.0` (VCV rule: MAJOR = Rack major = 2; it is NOT "fixed"
  to the 1.4.x milestone number). Makefile `VERSION` must match `plugin.json` version.
  Field-by-field manifest validation: tags valid, module slug/name correct, no trademarked
  strings.

### Slug / brand lock (PKG-03)
- **D-04:** Plugin slug `ForgeAudio-AnalogSeries` and module slug `ForgeAnalogLFO` are
  **final** (permanent once published). Execution runs a "Forge" brand/slug collision
  check against the VCV Library plugins list (and community) and only flags if a real
  conflict is found. No rename planned.

### CI scope (TEST-06)
- **D-05:** CI remains the **Rack-free doctest suite** on ubuntu/macos/windows (Phase 22
  D-09 intent — CI fetches no Rack SDK). TEST-06 = that matrix green on push. The real
  plugin build is verified locally in this phase via `make dist` (PKG-05), not in CI.
  → **BUT TEST-06 is currently UNMET:** the matrix is red on Windows and Linux. The two
  root causes below must be fixed for this phase to pass.

### CI cross-platform fixes (blocks TEST-06) — diagnosed, report-only, not yet applied
- **D-06 — Failure 1: `M_PI` undeclared on the Windows test build.**
  Root cause: `M_PI` is non-standard; MinGW g++ only exposes it with `_USE_MATH_DEFINES`,
  which is defined nowhere in the tree. Used in `src/dsp/Waveshape.hpp` (40,72,75,92),
  `src/dsp/DriftEngine.hpp` (66,70,74,78,83), `src/dsp/LfoCore.hpp` (229),
  `tests/test_extraction.cpp` (164), and `src/AnalogLFO.cpp` (421–426,981,1043–1075).
  **Affects the Rack-free TEST target only — NOT the shipped plugin:**
  `../Rack-SDK/compile.mk:45` injects `-D_USE_MATH_DEFINES` on the Windows plugin build
  (Rack's own `componentlibrary.hpp` depends on it), so `make`/`make dist` on Windows
  compile fine. Only the direct-g++ CI test leg lacks the define. Linux passes Failure 1
  only because libstdc++ leaks `M_PI` (non-portable luck).
  **DECISION — fix approach: introduce a shared rack-free `forge::kPi` constexpr** and
  replace every `M_PI` use (DSP headers + `AnalogLFO.cpp` + tests). Removes the
  non-standard macro entirely, include-order-independent, no build-flag reliance.
  **Numerically safe:** `constexpr double kPi = 3.14159265358979323846` rounds to the
  same IEEE-754 double as `M_PI` (`0x400921FB54442D18`), so `(float)kPi == (float)M_PI`
  and `(double)kPi == M_PI` bit-for-bit — golden fixtures are NOT perturbed. (Rejected
  alternative: `-D_USE_MATH_DEFINES` in TEST_CXXFLAGS + the CI Windows g++ line — smaller
  but keeps a non-standard macro; user preferred the constexpr.)

- **D-07 — Failure 2: Linux golden drift exceeds the 1e-5 tolerance.**
  Root cause: the drift path uses `std::normal_distribution<float>`
  (`src/dsp/DriftEngine.hpp:46, :98`). The Xoroshiro128+ *uniform* stream is bit-identical
  cross-platform, but `std::normal_distribution` is **not portable** — libc++ (macOS),
  libstdc++ (Linux), MinGW (Windows) use different transform algorithms and diverge
  algorithmically from the first draw. The golden scenario is drift-ON
  (`tests/test_golden.cpp:60`, `in.drift=0.5f`); the `1e-5` absolute tolerance
  (`test_golden.cpp:73,95`) was written assuming it could absorb normal_distribution
  differences — a false premise (the divergence is on the order of drift depth, not ULPs).
  This is **NOT benign codegen noise and NOT a DSP regression**; same-platform determinism
  is bit-exact and separately pinned in `test_invariants.cpp`; FP is already pinned
  (`-ffp-contract=off`, no `-ffast-math`).
  **DO NOT widen the epsilon** — covering algorithmic Gaussian divergence needs an epsilon
  so large it stops catching regressions.
  **DECISION — fix approach: restructure the golden.** Cross-platform leg replays the
  scenario **drift-OFF** (deterministic; small libm epsilon ~1e-6 for sin/cos variance)
  as a real cross-platform regression guard; the drift-ON bit-exact golden runs
  **macOS-only** (canonical OS). Requires generating drift-off golden fixtures. **Test-only
  — no shipped-audio change.** (Rejected here: replacing `std::normal_distribution` with a
  portable hand-rolled Gaussian — it is the strongest fix but CHANGES shipped drift audio,
  violating the milestone feature-freeze; deferred to a future DSP milestone. Also rejected:
  simply skipping the drift-on golden off-canonical — weaker guard.)

### Claude's Discretion
- Exact filename/location of the `forge::kPi` header (e.g., `src/dsp/MathConst.hpp`), the
  precise drift-off libm epsilon value, and the drift-off golden fixture generation
  procedure — planner/executor to choose, consistent with the decisions above.
- Order of operations within the phase (manifest edits vs CI fixes vs dist verification).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements & roadmap
- `.planning/REQUIREMENTS.md` — PKG-02/03/05 + TEST-06 definitions; "Do NOT" table
  (version-fix trap, slug-rename trap); traceability table.
- `.planning/ROADMAP.md` §"Phase 26" — goal, success criteria, open decisions
  (minRackVersion, Forge collision check).

### Manifest & packaging
- `plugin.json` — the manifest under validation (slug, version, minRackVersion, empty URLs).
- `Makefile` — `VERSION` must match manifest; `dist`/`install` come from Rack's `plugin.mk`;
  `TEST_CXXFLAGS` at L38 (the Rack-free test flags).
- `../Rack-SDK/compile.mk` §Windows branch (L45) — proves `-D_USE_MATH_DEFINES` is injected
  for the shipped Windows plugin build (why Failure 1 is tests-only).

### CI & test harness
- `.github/workflows/test.yml` — the 3-OS matrix (D-09 Rack-free CI); Windows direct-g++ leg.
- `tests/test_golden.cpp` — golden replay + the platform epsilon split under restructure.
- `tests/golden/freerun_seeds.txt` — canonical capture params/seeds (macOS canonical OS).
- `src/dsp/DriftEngine.hpp` (46, 98) — `std::normal_distribution` non-portability source.
- `tests/test_invariants.cpp` — same-platform determinism guard (already covers per-OS
  drift determinism; do not duplicate).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- CI matrix already exists (`.github/workflows/test.yml`) — extend, don't recreate. Just
  needs to go green (M_PI define/constexpr reachable in the Windows leg; golden restructure).
- `make dist` / `make install` provided by Rack's `plugin.mk` — no custom dist target needed;
  PKG-05 is "run it and verify the zip contents," not "author a packaging script."
- `forge::` DSP headers are already rack-free and self-contained — a `forge::kPi` constexpr
  header fits the existing `src/dsp/*.hpp` convention.

### Established Patterns
- Rack-free test discipline (Phase 22 D-09): the test target links no libRack and sets no
  `-I$(RACK_DIR)/include`. Any M_PI fix for the test build must NOT reintroduce a Rack
  dependency — the `forge::kPi` constexpr honors this; a compiler `-D` in test flags would too.
- Golden bit-exactness is the regression contract (D-08). Any change touching DSP source must
  preserve `(float)` results bit-for-bit — the `kPi` value is chosen specifically to do so.

### Integration Points
- `plugin.json` URLs + version ↔ `Makefile VERSION` (must agree).
- `src/dsp/MathConst.hpp` (new) ↔ every DSP header + `AnalogLFO.cpp` + `test_extraction.cpp`.
- New drift-off golden fixtures ↔ `tests/test_golden.cpp` restructured comparison.

</code_context>

<specifics>
## Specific Ideas

- User explicitly wants a **report-first** posture on the CI failures: diagnosis was
  delivered and approved before any code change. Executor should still land each fix as a
  verifiable red→green step (Windows/Linux CI legs going green is the acceptance signal).
- User's stated preference on Failure 1: avoid relying on a macro — hence `forge::kPi`.

</specifics>

<deferred>
## Deferred Ideas

- **Portable Gaussian rewrite** (replace `std::normal_distribution` with a hand-rolled
  Box–Muller for bit-exact drift on all platforms). Strongest cross-platform fix but CHANGES
  shipped drift audio → violates the v1.4 feature-freeze. Defer to a future DSP milestone if
  ever desired.
- **`pluginUrl` → Notion manual** swap once Phase 27 publishes the manual (presentation nicety;
  not required for PKG-02).

None of the above block Phase 26.

</deferred>

---

*Phase: 26-vcv-library-compliance-packaging*
*Context gathered: 2026-07-09*
