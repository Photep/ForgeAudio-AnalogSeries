---
phase: 22-test-harness-foundation
plan: 01
subsystem: testing
tags: [doctest, cpp17, makefile, test-harness, vcv-rack]

# Dependency graph
requires: []
provides:
  - "Vendored single-header doctest 2.4.11 framework (tests/doctest.h) with pinned-version provenance"
  - "Single doctest implementation TU (tests/main.cpp) providing main()"
  - "Additive Rack-free `make test` target compiling tests/*.cpp with -Isrc -Itests -ffp-contract=off, no libRack"
  - "Passing smoke test proving the two-TU harness links, discovers, and runs"
  - "build-test/ gitignored; tests/golden left tracked for future regression baselines"
affects: [22-02, 22-03, 22-04, "DSP core extraction", "golden-output regression", "CI matrix"]

# Tech tracking
tech-stack:
  added: [doctest 2.4.11 (vendored single header)]
  patterns:
    - "Header-only test framework, exactly one TU defines DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN"
    - "Additive TEST_-namespaced Makefile block below plugin.mk include (zero perturbation to plugin build)"
    - "Test target links no libRack and uses no -I$(RACK_DIR)/include"

key-files:
  created:
    - tests/doctest.h
    - tests/main.cpp
    - tests/test_smoke.cpp
  modified:
    - Makefile
    - .gitignore

key-decisions:
  - "Vendored doctest 2.4.11 (latest 2.4.x) from official release tag with top-of-file provenance comment; no submodule"
  - "Test target uses -ffp-contract=off and omits -ffast-math for cross-platform float bit-stability (Pitfall 6 groundwork)"

patterns-established:
  - "Pattern 3 (doctest single-impl TU): only main.cpp defines the impl macro; all other TUs just #include doctest.h"
  - "Additive build target: TEST_-namespaced Makefile vars appended below plugin.mk so make/dist/install are unchanged"

requirements-completed: [TEST-01]

# Metrics
duration: 3min
completed: 2026-06-14
---

# Phase 22 Plan 01: Test Harness Foundation Summary

**Rack-free `make test` target running a vendored doctest 2.4.11 harness across two translation units, fully additive — the plugin build is untouched and links no libRack.**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-06-14T08:38:42Z
- **Completed:** 2026-06-14T08:41:06Z
- **Tasks:** 3
- **Files modified:** 5 (3 created, 2 modified)

## Accomplishments
- Vendored `tests/doctest.h` (doctest 2.4.11, MIT, from the official `doctest/doctest` v2.4.11 release) with a top-of-file provenance comment (source URL + pinned version); no git submodule added.
- Added `tests/main.cpp` as the sole `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` translation unit, providing `main()`.
- Appended a purely additive, `TEST_`-namespaced `make test` recipe below `include $(RACK_DIR)/plugin.mk` — `-std=c++17 -Isrc -Itests -ffp-contract=off`, no `-I$(RACK_DIR)/include`, no libRack link.
- Added a trivial passing smoke test (`tests/test_smoke.cpp`) proving two-TU linking, discovery, and reporting work.
- Confirmed the load-bearing acceptance criterion: `make test` exits 0 (1 test case / 1 assertion passed) AND `make` rebuilds the plugin (`plugin.dylib`) unchanged.

## Task Commits

Each task was committed atomically:

1. **Task 1: Vendor doctest.h + single-impl bootstrap TU** - `8583442` (feat)
2. **Task 2: Additive `make test` recipe + gitignore build-test/** - `8056b28` (build)
3. **Task 3: Trivial passing smoke test** - `d43379b` (test)

**Plan metadata:** (final docs commit — see below)

## Files Created/Modified
- `tests/doctest.h` - Vendored single-header doctest 2.4.11 framework with provenance comment
- `tests/main.cpp` - The ONE TU defining `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`
- `tests/test_smoke.cpp` - `TEST_CASE("harness smoke: 1+1==2")` proving the harness runs
- `Makefile` - Appended additive `TEST_`-namespaced `make test` block below the plugin.mk include
- `.gitignore` - Added `build-test/`; `tests/golden` deliberately left tracked

## Decisions Made
- Vendored doctest 2.4.11 (confirmed latest 2.4.x at impl time; version macros `MAJOR=2 MINOR=4 PATCH=11` present in the header, MIT-licensed by Viktor Kirilov).
- Test target compiled with `-ffp-contract=off` and without `-ffast-math` for cross-platform float bit-stability — groundwork for the later golden-output regression gate (RESEARCH Pitfall 6).

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None. The `make test && make` chain reported "Nothing to be done for `all'" on the first plugin invocation because the plugin was already up to date; forcing a recompile (`touch src/AnalogLFO.cpp && make`) confirmed the plugin compiles and links `plugin.dylib` cleanly with the unchanged plugin.mk flags — proving the additive test block does not perturb the plugin build.

## Verification Evidence
- `make test`: `[doctest] test cases: 1 | 1 passed | 0 failed` → `Status: SUCCESS!`, exit 0.
- `make` (forced recompile): produced `plugin.dylib`, exit 0 — plugin build unchanged.
- No `-I$(RACK_DIR)/include` in the recipe (the two grep hits are in explanatory comments only); no libRack on the test link line.
- `.gitignore` contains `build-test/` and no `tests/golden` ignore rule.
- No git submodule added (`.gitmodules` absent).

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- The Rack-free doctest harness skeleton EXISTS (Constraint A from research is satisfied) — Plan 02+ can now move the DSP core into `src/dsp/*.hpp` and add `BlockDriver.hpp`, invariant tests, and golden regression against this harness.
- The `TEST_HEADERS := $(wildcard src/dsp/*.hpp) ...` glob already resolves to empty harmlessly until later plans create the core headers, so adding them requires no Makefile change.

## Self-Check: PASSED
- Files: tests/doctest.h, tests/main.cpp, tests/test_smoke.cpp, Makefile, .gitignore — all FOUND.
- Commits: 8583442, 8056b28, d43379b — all FOUND.

---
*Phase: 22-test-harness-foundation*
*Completed: 2026-06-14*
