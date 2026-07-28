---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
fixed_at: 2026-07-28T00:00:00Z
review_path: .planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-REVIEW.md
iteration: 1
findings_in_scope: 13
fixed: 13
skipped: 0
status: all_fixed
---

# Phase 29: Code Review Fix Report

**Fixed at:** 2026-07-28
**Source review:** `.planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-REVIEW.md`
**Iteration:** 1

**Summary:**
- Findings in scope: 13 (3 Critical + 10 Warning; `fix_scope: critical_warning`, so the 5 Info findings were not attempted)
- Fixed: 13
- Skipped: 0

The review's thesis was that all three guards are **fail-open**: green lights
scoped to a hand-written list or a constant-folded expression, with no mechanism
that notices when the scope shrinks to nothing. Every fix below was validated the
way the phase demanded of itself — by an **observed red** on a scratch copy,
reverted afterwards — rather than by reasoning. The working tree is clean and no
scratch perturbation survives.

## Verification

All run on the final tree, after a `rm -rf build plugin.dylib build-test`:

| Gate | Result |
|---|---|
| `make test` | 67 cases, 0 failed (2 615 121 assertions). Was 64; +3 new cases from WR-08/WR-09 |
| `make strict` | `strict C++11 gate: PASS`, exit 0 |
| `make guards` | all three guards PASS, `guard suite: PASS`, exit 0 |
| `make` (plugin) | exit 0, `plugin.dylib` links |
| `git diff v2.0.1 --` over shipped LFO paths | clean |

**Guardrails honoured:**
- `Makefile` **unmodified** — `TEST_CXXFLAGS` untouched.
- `.github/workflows/test.yml` **append-only: 19 added / 0 removed**; its verbatim `TEST_CXXFLAGS` duplicate untouched.
- No shipped LFO source, test or golden fixture modified. `src/AnalogLFO.cpp`, all eleven closure headers, `tests/BlockDriver.hpp`, `tests/test_golden.cpp` and every `tests/golden/*.f32` are byte-identical to `v2.0.1`.
- `src/dsp/FROZEN.sha256` and `tests/golden/SHA256SUMS` unmodified — no digest bump was needed because nothing pinned was touched.
- The three documented design intents (canary's unused symbol, `VcoBlockDriver.hpp` duplication, `step()` returning `0.f`) were left alone.

## Fixed Issues

### CR-03: The compile canary constant-folds the in-class `static constexpr` failure class away

**Files modified:** `src/vco_compile_canary.cpp`, `tests/check_canary.sh`
**Commit:** `e866643`

The canary fed `VcoInputs` only compile-time constants; the one runtime-derived
value was the loop **trip count**, which nothing inside `step()` depends on.

**Reproduced before fixing.** With `static constexpr float kTable[4]` indexed by
`in.pitchCV` injected into `VcoCore.hpp` and the exact CI compile
`c++ -std=c++11 -O3 -Isrc -c src/vco_compile_canary.cpp`: **no `kTable` symbol of
any kind** in the object, while `T __ZN5forge21vcoCompileCanaryProbeEi` — the
symbol `[2/5]` greps for — was still present.

**Fix:** every `VcoInputs` DSP field is now derived from the runtime parameter
`i`, and `pitchCV` varies per loop iteration.

**Fix (guard) — `[2b/5]`:** asserts the property `[2/5]` only claimed to test. It
builds a perturbed copy of `dsp/VcoCore.hpp` carrying one in-class `static
constexpr` table **per float field of `VcoInputs`**, each indexed at runtime by
its own field, and compiles a copy of the real canary against it at `-O3`. A
vanished table names the field being fed a constant. The field list is derived
from the header, so later phases are covered on landing. A compiler-capability
precondition downgrades to INFO if a compiler erases even a genuinely
runtime-indexed table.

One subtlety worth recording: the canary is **copied** next to the perturbed
header before compiling. A quoted `#include` resolves against the including
file's own directory before any `-I` path, so compiling it in place picked up the
real `src/dsp/VcoCore.hpp` and made `[2b/5]` vacuously green — the same failure
mode, one level up. That was caught during development, not by inspection.

**Empirically verified, final tree:**
- Fixed canary + landmine at `-O3` → `l__ZN5forge7VcoCore6kTableE.const` present. The odr-use survives the optimizer, so MinGW's linker has an undefined reference to fail on.
- Pre-fix canary + same landmine → no `kTable` symbol; probe symbol still present.
- `[2b/5]` observed red: reverting only the canary makes it name all eight fields.

Local reproduction of the **link** failure itself is not possible — only Apple
clang 16 is available here, which materializes the construct as a per-TU local
symbol (`s`) and links cleanly, exactly as the canary banner documents. What is
proven locally is the thing that was broken: the odr-use now reaches the object
instead of being destroyed at the compiler.

### CR-01: `check_includes.sh` [1/7] misses any transitive VCO leak

**Files modified:** `tests/check_includes.sh`
**Commit:** `c044d26`

Inverted the hand-written 25-file allowlist to a **denylist** (everything under
`src/`, `tests/`, `tools/` is LFO-side unless explicitly VCO-side) and added
`resolve_quoted_include` + `expand_include_closure`, a worklist walk that
resolves quoted includes the way the compiler does and scans the closure at any
depth. `[6/7]` gained a **two-hop** negative control whose fixture names no VCO
token in its own text — and which fails loudly if it ever becomes
directly-matching, since a direct fixture validates the regex rather than the
scope.

The old list was already stale: it covered 25 files where the derivation finds
29 (`tests/Sha256.hpp`, `tests/main.cpp`, `tests/doctest.h`,
`tests/test_lfo_guardrail.cpp` were never scanned).

**Observed red** on the reviewer's exact bypass (new `src/dsp/PluginHelper.hpp`
pulled into `src/plugin.cpp`) and on a deeper two-hop chain through
`src/AnalogLFO.cpp`.

### CR-02: `check_frozen.sh` has no coverage floor

**Files modified:** `tests/check_frozen.sh`
**Commit:** `7bb1dd1`

Two independent guards, because a floor alone misses a file that was never pinned
and a sweep alone misses a removal outside its scope:
`FROZEN_EXPECTED_ENTRIES=15` / `GOLDEN_EXPECTED_ENTRIES=6` exact entry counts,
plus a completeness sweep over `src/dsp/*.hpp` (minus VCO headers) + `src/AnalogLFO.cpp`,
and over `tests/golden/*.f32`. The sweep walks the **directory**, not a list, so a
header added by a later phase must be pinned or named VCO-side. That also closes
the "new unpinned header" half of CR-01.

**Observed red** on all three: the reviewer's exact `src/AnalogLFO.cpp` manifest-line
deletion (caught by both guards), a new unpinned `src/dsp` header, and a deleted
golden fixture line.

### WR-01: fabricated scan count, silent skip of missing files

**Files modified:** `tests/check_includes.sh` · **Commit:** `e99955b`

The detector runs inside `$( )`, so it cannot set `fail` or bump a counter for
its caller — both facts now return through the filesystem. Real opened-file count
reported; a listed-but-missing path `note_fail`s. Verified the count tracks
reality (31 → 30 when a header is deleted) and that the missing-file branch fires.

### WR-02: negative controls reported success for any compile failure

**Files modified:** `tests/check_canary.sh` · **Commit:** `8db6977`

Each control now requires its fixture to compile clean at `-std=c++17` first, and
requires the C++11 diagnostic to actually name the construct under test. Patterns
cover both clang and GCC phrasings. **Observed red:** with `VcoCore.hpp` broken by
a bogus `#include`, all four previously printed "OK: ... rejected by the C++11
pedantic gate"; they now fail with the real diagnostic.

### WR-03: ODR guard evaded by a next-line brace

**Files modified:** `tests/check_includes.sh` · **Commit:** `84a92f1`

Regex now allows the Allman brace and still excludes forward declarations for
free (`;` is neither `{`, `:` nor end-of-line), so there is no second exclusion
grep to keep in sync. Truth table verified directly. **Observed red** on an
Allman-brace second `forge::Inputs`.

### WR-04: hasher-placement constant check was case-sensitive

**Files modified:** `tests/check_includes.sh` · **Commit:** `ab9e521`

`grep -rniE '0x6a09e667|0xbb67ae85'`. **Observed red** on an uppercase-constant
header and on an H1-only header.

### WR-05: manifest parsing dropped an unterminated last line and was not CR-safe

**Files modified:** `tests/check_frozen.sh` · **Commit:** `4a29b56`

All manifest reads — both digest loops, both CR-02 completeness sweeps and the
`[3/3]` control's `awk` — now go through `manifest_lines()`, which strips CR and
guarantees a final newline; `#` and blank lines are skipped before the counter.
**Observed old-vs-new** on all three: no trailing newline (14 entries → 15), CRLF
manifests (21 spurious MISSING failures → PASS), comment line (spurious MISSING →
PASS).

### WR-06: [7/7] accepted a guard that was only *mentioned*

**Files modified:** `tests/check_includes.sh` · **Commit:** `fc42074`

Comment lines and step titles are stripped; the path must appear in an executable
position. **Observed red** on a workflow where `check_canary.sh` survives only in
a `# TODO:` comment and a `- name:` title — the old script called that "wired
into CI".

### WR-07: `make guards` duplicated the CI list with no execution coverage

**Files modified:** `.github/workflows/test.yml`, `tests/check_includes.sh`
**Commit:** `0e722bf`

Added a `make guards` CI step (**append-only, 19/0**; the three direct steps are
deliberately kept so a failure still names which guard failed). `[7/7]` now
requires the workflow to run `make guards` and requires every guard to be in
`GUARD_SCRIPTS` (parsed with backslash-continuation support); a guard reached only
via `make guards` also counts as wired, so the two lists cannot drift in either
direction. **Observed red** on both halves.

### WR-08: `Sha256::update()` silently discarded data after `hex()`

**Files modified:** `tests/Sha256.hpp`, `tests/test_lfo_guardrail.cpp`
**Commit:** `0a12a4f`

`assert(!finalized)` plus a `sealed()` accessor so the one-way state machine can
be pinned by a test without tripping the assert. New case pins the whole
contract. **Verified both directions:** the pre-fix silent-prefix result
(`d1 == sha256("abc") != sha256("abcdef")`) reproduced, and the call now aborts
with "the digest is already sealed".

### WR-09: CR-stripping branch had zero coverage and no caller

**Files modified:** `tests/test_lfo_guardrail.cpp` · **Commit:** `1053848`

Two cases: a single-chunk LF/CRLF pair (asserting the raw variant still
*distinguishes* them, otherwise the case is vacuous), and a ~20 KB pair crossing
the 8192-byte read-chunk boundary, which the single-chunk case cannot cover.
**Mutation-checked:** neutering the `0x0d` filter fails both. Scratch files are
removed; the tree stays clean.

### WR-10: VCO detection keyed on an unenforced filename convention

**Files modified:** `tests/check_canary.sh`, `tests/check_includes.sh`
**Commit:** `c18e0a0`

New `[5b/5]`: every `dsp/` header reached by the canary or a VCO header must be
`Vco*`-named or one of the four D-05 frozen shared headers — which also turns a
VCO/LFO-internals coupling into a written-down D-05 decision. `VCO_TOKEN` matching
is now case-insensitive. **Observed red** on all three controls, including a
lowercase `dsp/vcocore.hpp` include in `src/AnalogLFO.cpp` that the previous
script reported as "zero VCO includes".

## Skipped Issues

None.

## Notes for the operator

**Two items worth a human eye, neither a defect:**

1. **`[2b/5]` runs a C++ compile on every `make guards`.** It adds roughly one
   extra `-O3` compile of the canary per run. If local guard latency ever
   matters, that is the knob — but it is the only thing standing between the
   repo and a silently-green MinGW leg.

2. **`FROZEN_EXPECTED_ENTRIES = 15` and `GOLDEN_EXPECTED_ENTRIES = 6` are now a
   deliberate bump protocol.** Phase 34's sanctioned `DriftEngine.hpp` digest bump
   does **not** change the count, so it is unaffected. Any phase that *adds* a
   pinned file must bump the constant in the same commit — by design.

**Not attempted (out of `fix_scope`):** IN-01 (`head -1` under `pipefail`),
IN-02 (block-comment stripping), IN-03 (`HASHER` naming), IN-04 (`local` in
`check_negative_control` — incidentally fixed as part of WR-02's rewrite of that
function), IN-05 (`VcoBlockDriver::run()` negative sample count).

---

_Fixed: 2026-07-28_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
