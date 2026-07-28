---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
plan: 02
subsystem: testing
tags: [sha256, doctest, golden-regression, integrity-tripwire, fips-180-4, cpp11]

# Dependency graph
requires:
  - phase: 22-test-harness
    provides: "tests/test_golden.cpp six-fixture LFO golden replay, tests/golden/freerun_*.f32, the Rack-free `make test` target and its tests/*.cpp + tests/*.hpp auto-globs"
  - phase: 26-cross-platform-goldens
    provides: "the portable drift-off fixtures (freerun_*_driftoff.f32) that make the replay meaningful on all three CI operating systems"
provides:
  - "tests/Sha256.hpp — a vendored, test-scope SHA-256 (FIPS PUB 180-4) with five inline forge:: entry points, including a CR-normalizing file variant for the Windows CRLF hazard"
  - "D-04 golden byte lock: the SHA-256 of all six tests/golden/freerun_*.f32 pinned as source literals, checked on every `make test` run on all three CI operating systems"
  - "A permanent negative control proving the lock goes red on a single flipped bit"
  - "tests/golden/SHA256SUMS — human-runnable mirror of the pinned digests"
affects: [29-04-frozen-header-manifest, 30-vcocore-registration, 34-audio-rate-analog-engine, 36-goldens-ci-library-update]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Integrity tripwires live in a NEW test TU so the witness they harden (tests/test_golden.cpp) stays byte-unchanged and can itself be hash-pinned"
    - "Expected digests are source literals, never a regenerable data file"
    - "Every guard ships with a permanent negative control in the same commit"

key-files:
  created:
    - tests/Sha256.hpp
    - tests/test_lfo_guardrail.cpp
    - tests/golden/SHA256SUMS
  modified: []

key-decisions:
  - "D-04 golden digests are pinned as source literals in tests/test_lfo_guardrail.cpp, not in a data file — changing a golden therefore requires a reviewed code diff"
  - "The guard lives in a new TU; tests/test_golden.cpp is byte-unchanged (R-3)"
  - "SHA-256 vendored in-repo rather than added as a dependency, and scoped to tests/ only — never src/, never the shipped C++11 build graph (V6)"
  - "The hasher is validated by a permanent negative control (published vectors + a one-byte perturbation), not by a green run"
  - "tests/golden/SHA256SUMS excludes freerun_seeds.txt — a text file subject to the Windows CRLF hazard, pinned instead by plan 29-04's CR-normalizing manifest"

patterns-established:
  - "New-TU hardening: harden a witness from outside it, never by editing it"
  - "Digest-as-literal: an integrity baseline that can only move through a code review"
  - "Negative control as a permanent test case, not a one-off manual check"

requirements-completed: [TEST-04]

coverage:
  - id: D1
    description: "One `make test` invocation both replays the six shipped LFO goldens and proves their bytes are unchanged (ROADMAP criterion 1)"
    requirement: "TEST-04"
    verification:
      - kind: integration
        ref: "tests/test_lfo_guardrail.cpp#lfo guardrail: golden .f32 bytes are unchanged (D-04 checksum lock)"
        status: pass
      - kind: integration
        ref: "tests/test_golden.cpp#golden* (6 pre-existing replay cases, byte-unchanged)"
        status: pass
      - kind: other
        ref: "make test — 64 cases / 2,615,099 assertions / 0 failed, clean build with zero warnings"
        status: pass
    human_judgment: false
  - id: D2
    description: "The hash lock is validated by observed failure modes — three published FIPS 180-4 vectors and a one-byte-perturbed in-memory copy of a real golden — not merely by a green run"
    requirement: "TEST-04"
    verification:
      - kind: unit
        ref: "tests/test_lfo_guardrail.cpp#lfo guardrail: SHA-256 hasher matches the published empty-string vector"
        status: pass
      - kind: unit
        ref: "tests/test_lfo_guardrail.cpp#lfo guardrail: SHA-256 hasher matches the published abc vector"
        status: pass
      - kind: unit
        ref: "tests/test_lfo_guardrail.cpp#lfo guardrail: SHA-256 hasher matches the published two-block vector"
        status: pass
      - kind: unit
        ref: "tests/test_lfo_guardrail.cpp#lfo guardrail: golden hash lock detects a single-byte change (negative control)"
        status: pass
      - kind: other
        ref: "shasum -a 256 cross-check of all three published vectors against the pinned literals"
        status: pass
    human_judgment: false
  - id: D3
    description: "No hashing code exists under src/ — the tripwire never enters the shipped C++11 plugin build graph (V6 / T-29-09)"
    verification:
      - kind: other
        ref: "ls src/dsp/Sha256.hpp src/Sha256.hpp 2>/dev/null | wc -l -> 0"
        status: pass
      - kind: other
        ref: "git diff f539475..HEAD -- Makefile .github/workflows/test.yml -> exit 0 (no build/CI edit needed; tests/*.hpp glob covers it)"
        status: pass
    human_judgment: false
  - id: D4
    description: "tests/test_golden.cpp and all six .f32 fixtures are byte-identical to tag v2.0.1 (R-3 / R-6 / T-29-05 / T-29-07)"
    verification:
      - kind: other
        ref: "git diff --exit-code v2.0.1 -- tests/test_golden.cpp tests/BlockDriver.hpp tests/golden/freerun_*.f32 tests/golden/freerun_seeds.txt -> exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "tests/golden/SHA256SUMS verifies clean with the stock checksum tool from the repo root and agrees digest-for-digest with the pinned literals"
    requirement: "TEST-04"
    verification:
      - kind: other
        ref: "shasum -a 256 -c tests/golden/SHA256SUMS -> 6x OK, exit 0"
        status: pass
      - kind: other
        ref: "cut -d' ' -f1 tests/golden/SHA256SUMS | while read h; do grep -q \"$h\" tests/test_lfo_guardrail.cpp || exit 1; done -> exit 0"
        status: pass
    human_judgment: false

# Metrics
duration: 6 min
completed: 2026-07-28
status: complete
---

# Phase 29 Plan 02: LFO Golden Byte Lock (D-04) Summary

**A vendored FIPS 180-4 SHA-256 in `tests/Sha256.hpp` plus a new `tests/test_lfo_guardrail.cpp` that pins all six shipped-LFO golden digests as source literals — so regenerating a golden to mask a regression now requires a visible code diff, and the lock itself is proven to go red on a single flipped bit.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-07-28T05:28:10Z
- **Completed:** 2026-07-28T05:35:06Z
- **Tasks:** 3
- **Files created:** 3 (zero modified)

## Accomplishments

- **The "just regenerate the goldens" escape hatch is closed.** The six existing replays in `tests/test_golden.cpp` catch behavioral drift only; they say nothing if someone reruns `make capture` and re-baselines the fixtures. The D-04 lock pins each fixture's SHA-256 as a hard-coded literal, so moving a golden now requires editing the guard in the same reviewed diff.
- **The guard was hardened from outside the witness.** `tests/test_golden.cpp` is byte-unchanged versus tag `v2.0.1` (R-3). Editing it to harden it would have let one edit weaken both the replay and the guard on the replay; it stays frozen and is hash-pinned by plan 29-04.
- **The lock is validated, not merely green.** Three published FIPS 180-4 vectors (empty, `abc`, and the 56-byte two-block message that catches naive padding) plus a permanent negative control that flips one bit of an in-memory copy of a real golden and requires the digest to move. A hasher that returned a constant, an empty string, or a truncated digest fails these.
- **Zero build and zero CI edits.** `TEST_SOURCES`/`TEST_HEADERS` and the Windows leg's literal `tests/*.cpp` already glob the new files, so `TEST_CXXFLAGS` (R-4) and `.github/workflows/test.yml` were never touched. The guard runs inside the same `make test` invocation as the LFO replays, on all three CI operating systems, with no external hashing tool (`sha256sum` does not exist on macOS).
- **`make test`: 64 cases / 2,615,099 assertions / 0 failed**, clean rebuild with zero compiler warnings.

## Task Commits

Each task was committed atomically:

1. **Task 1 (RED): failing SHA-256 vector guards** — `b413254` (test)
2. **Task 1 (GREEN): vendored test-scope SHA-256** — `d0ac596` (feat)
3. **Task 2: D-04 golden byte lock + negative control** — `0b9402d` (feat)
4. **Task 3: `tests/golden/SHA256SUMS` manifest** — `b95c396` (chore)

_Task 1 was a TDD task: the test TU was committed failing to build (`'Sha256.hpp' file not found`) before the header existed. No REFACTOR commit was needed — the GREEN implementation was already warning-clean and C++11-clean._

## Files Created/Modified

- `tests/Sha256.hpp` (created) — Streaming SHA-256 from FIPS PUB 180-4. `forge::Sha256` (`update`/`hex`, idempotent finalize guarded by a `finalized` flag) plus five inline free functions: `detail::hashFileImpl`, `sha256HexBytes`, `sha256Hex`, `sha256HexFile`, `sha256HexFileLfNormalized`. The 64-entry round-constant table is a function-local `static const` inside the compression function (no unused-variable diagnostic in TUs that include but do not use the header; no ODR question). Banner states the integrity-tripwire scope and the never-move-to-`src/` placement rule.
- `tests/test_lfo_guardrail.cpp` (created) — Seven doctest cases prefixed `"lfo guardrail: "`: three published-vector cases, a missing-file case, the six-fixture D-04 checksum lock, the single-byte-perturbation negative control, and a 32768-byte length assertion per fixture. Digests are compared as exact strings; `doctest::Approx` appears nowhere.
- `tests/golden/SHA256SUMS` (created) — Six pure digest lines, repo-root-relative paths, generated mechanically with `shasum -a 256`. Verifiable from the repo root with `shasum -a 256 -c` (macOS) or `sha256sum -c` (Linux).

## Decisions Made

- **Digests as source literals, not a data file.** A `SHA256SUMS`-style manifest alone could be regenerated in the same careless motion that regenerates the fixtures. A literal in test source cannot move without a code diff. `tests/golden/SHA256SUMS` ships as an explicitly secondary convenience mirror.
- **A new translation unit, not an edit to `test_golden.cpp`.** `test_golden.cpp` is the shipped LFO's only behavioral witness; it stays byte-unchanged and is pinned by 29-04.
- **SHA-256 vendored in-repo, `tests/` only.** No new dependency (the phase's package-legitimacy surface stays empty), and no hashing code in the shipped C++11 plugin build graph. Written C++11-clean anyway so a future relocation is cheap.
- **`sha256HexFileLfNormalized` shipped now, consumed later.** Binary `.f32` fixtures are hashed raw, so they have no CRLF exposure. The CR-stripping variant exists for plan 29-04's text-file manifest, because GitHub's `windows-latest` runner checks text files out with CRLF and this repository has no `.gitattributes` (P-3).
- **Digest lookup by path, not by table index**, in the negative control — a table reorder cannot silently mis-pair a fixture with someone else's digest.
- **`freerun_seeds.txt` deliberately excluded from `SHA256SUMS`** — it is a text file subject to the same CRLF hazard, and belongs in 29-04's CR-normalizing manifest instead.

## Deviations from Plan

None - plan executed exactly as written.

Both "recompute, do not trust the plan" gates were run before anything was written, and both agreed exactly with the plan text:

- All three published FIPS 180-4 vectors reproduced byte-for-byte via `printf '%s' … | shasum -a 256`.
- All six golden digests reproduced identically to the values recorded in 29-RESEARCH.md, so no fixture has moved since research. No re-pinning was required and no discrepancy needed surfacing.

**Total deviations:** 0
**Impact on plan:** None. All five hard prohibitions held: `tests/test_golden.cpp` untouched, no file under `tests/golden/` edited/regenerated/moved (only `SHA256SUMS` added), `make capture` never run, no hashing code under `src/`, `TEST_CXXFLAGS` and `.github/workflows/test.yml` untouched, and `doctest::Approx` used nowhere.

## LFO Non-Regression Guardrail

This plan is entirely additive and touched no shared header, no `src/` file, no `Makefile` line, and no CI workflow line. Verified against tag `v2.0.1`:

- `git diff --exit-code v2.0.1 -- tests/test_golden.cpp tests/BlockDriver.hpp tests/golden/freerun_{44100,48000,96000}{,_driftoff}.f32 tests/golden/freerun_seeds.txt` → **exit 0**
- `git status --porcelain Makefile .github/workflows/test.yml` → **empty**
- `git diff --exit-code <plan-base>..HEAD -- Makefile .github/workflows/test.yml` → **exit 0**
- `git diff --diff-filter=D --name-only <plan-base>..HEAD` → **empty** (no deletions)

The shipped LFO's six golden replays still pass (`-tc="golden*"` → 6/6) and are now accompanied by a proof that the bytes they replay against are unchanged.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern, or schema change at a trust boundary. The threat register's `mitigate` dispositions were all implemented as specified:

| Threat ID | Status |
|-----------|--------|
| T-29-05 (golden tampering) | Mitigated — D-04 literal lock, `make capture` prohibited in the freeze comment |
| T-29-06 (weakened hasher / deleted case) | Mitigated — published vectors + permanent negative control with a do-not-delete comment |
| T-29-07 (`test_golden.cpp` tampering) | Mitigated — byte-unchanged vs `v2.0.1`, pinned by 29-04 |
| T-29-08 (Windows CRLF) | Mitigated — binary fixtures hashed raw; `sha256HexFileLfNormalized` provided for 29-04 |
| T-29-09 (misuse of cryptography) | Mitigated — banner scope statement + placement rule; asserted by acceptance criterion, to be re-asserted by `tests/check_includes.sh` in 29-04 |
| T-29-SC (supply chain) | Accepted — zero packages installed |

## Known Stubs

None. Every entry point shipped in `tests/Sha256.hpp` is fully implemented and exercised by a test in the same commit range. `sha256HexFileLfNormalized` has no in-repo caller yet by design (29-04 consumes it), but it is not a stub — it is covered by the missing-file case and by the shared `detail::hashFileImpl` body that the raw file digests exercise on every run.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Ready for 29-03** (VCO compile canary) and **29-04** (frozen-header manifest, `check_frozen.sh`, `check_includes.sh`).
- 29-04 inherits two ready-made inputs from this plan: `forge::sha256HexFileLfNormalized` for the text-file manifest, and `tests/golden/SHA256SUMS` as the mirror it re-verifies. It also owns adding `tests/test_golden.cpp`, `tests/BlockDriver.hpp`, `tests/test_lfo_guardrail.cpp` and `tests/Sha256.hpp` to the D-05 hash manifest so the guard's own source is pinned.
- No blockers. No operator decision is outstanding for this plan.

---
*Phase: 29-vco-test-harness-lfo-non-regression-guardrail*
*Completed: 2026-07-28*

## Self-Check: PASSED

- Files claimed created exist on disk: `tests/Sha256.hpp` FOUND, `tests/test_lfo_guardrail.cpp` FOUND, `tests/golden/SHA256SUMS` FOUND.
- Commits claimed exist in git: `b413254` FOUND, `d0ac596` FOUND, `0b9402d` FOUND, `b95c396` FOUND.
- All task acceptance criteria re-run and passing (8 for Task 1, 8 for Task 2, 5 for Task 3).
- All six plan-level `<verification>` commands re-run: `make test` exit 0 / 64 passed; `-tc="lfo guardrail*"` exit 0 / 7 passed; `-tc="golden*"` exit 0 / 6 passed; `shasum -a 256 -c tests/golden/SHA256SUMS` exit 0 / 6x OK; `git diff --exit-code v2.0.1 --` frozen set exit 0; `git status --porcelain Makefile .github/workflows/test.yml` empty.
