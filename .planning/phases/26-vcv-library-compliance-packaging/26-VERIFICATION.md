---
phase: 26-vcv-library-compliance-packaging
verified: 2026-07-09T02:36:01Z
status: human_needed
score: 5/7 must-haves verified (1 legitimately deferred to Phase 28, 2 pending human/remote action)
overrides_applied: 0
deferred:
  - truth: "plugin.json authorUrl/pluginUrl/sourceUrl return HTTP 200 (reachability half of PKG-02 / roadmap SC1)"
    addressed_in: "Phase 28"
    evidence: "Phase 28 SC1: 'The GitHub repo is flipped to public ONLY after the Phase 25 purge verification passed' — the repo is confirmed PRIVATE today (gh repo view -> visibility PRIVATE, curl -> HTTP 404), and STATE.md documents the explicit hard sequencing constraint 'git-history font purge (Phase 25) completes + verifies while repo is PRIVATE before the public flip (Phase 28)'. The URL string itself is verified correct (matches git remote origin); only the flip is deferred."
human_verification:
  - test: "Push the 25 unpushed local commits (b550c86..2293b36, containing all four 26-01..26-04 plan commits) to origin/main and observe the GitHub Actions 'test' workflow run to completion on all three OS legs (ubuntu-latest, macos-latest, windows-latest)."
    expected: "All three legs report SUCCESS with 50 doctest cases / ~2.6M assertions passed, 0 failed — matching the local macOS result (50 cases, 2,615,027 assertions, 0 failures) confirmed during this verification."
    why_human: "The repo is 25 commits ahead of origin/main — `gh run list` shows the most recent ACTUAL CI run (28934233470, 2026-07-08, commit predating this phase's fixes) as a FAILURE. No CI run has ever executed against the M_PI removal (26-02) or the golden restructure (26-03) fix commits, because they have not been pushed. Root-cause code review and local macOS execution both support that the fix is correct, but 'the full test suite runs green in CI' (the literal phase goal and roadmap SC4) is a remote-system fact that cannot be confirmed by static analysis or local test runs alone — it requires an actual push + a completed Actions run. Pushing to origin/main is a state-changing action outside verifier scope."
  - test: "Confirm the six Rack API symbols the minRackVersion=2.0.0 floor depends on (ParamQuantity::getScaledValue, Module::isBypassed, createIndexSubmenuItem, drawLayer self-illumination, Window::getLastFrameDuration, Widget::addChildBelow) actually exist and behave identically in the Rack 2.0.0 SDK, not just the 2.6-era SDK the plugin is built against."
    expected: "Either (a) all six symbols resolve when compiled against Rack 2.0.0 SDK headers, validating the minRackVersion=2.0.0 floor, or (b) the floor is raised to the lowest version actually confirmed."
    why_human: "26-01's SUMMARY only checked two of the six APIs the code reviewer (26-REVIEW.md WR-01) identified as version-sensitive (getLastFrameDuration, createIndexSubmenuItem). The other four (getScaledValue, isBypassed, addChildBelow, drawLayer self-illumination) were never checked against a 2.0.0-era SDK. Only the 2.6-era ../Rack-SDK is available in this environment, so a genuine audit requires either downloading the Rack 2.0.0 SDK or consulting the Rack changelog/git history — outside static grep verification."
---

# Phase 26: VCV Library Compliance + Packaging Verification Report

**Phase Goal:** The manifest is validated and submission-ready, a verified `.vcvplugin` artifact is produced, and the full test suite runs green in CI.
**Verified:** 2026-07-09T02:36:01Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | plugin.json `authorUrl`/`pluginUrl`/`sourceUrl` populated with the correct GitHub URL (PKG-02, roadmap SC1 half) | VERIFIED | `cat plugin.json` — all three fields equal `https://github.com/Photep/ForgeAudio-AnalogSeries`, matches `git remote -v` origin exactly |
| 2 | plugin.json URLs return HTTP 200 (PKG-02, roadmap SC1 half) | DEFERRED (see frontmatter) | `curl -sSI` → HTTP 404; `gh repo view` → `"visibility":"PRIVATE"`. Legitimate, documented Phase 25→28 sequencing — not a Phase 26 defect |
| 3 | Manifest field-by-field validated: version stays 2.0.0, VERSION derives via jq in plugin.mk, tags whitelist-valid, slug charset-valid/immutable, no trademarked strings (PKG-03, roadmap SC2) | VERIFIED | `jq -r .version` = `2.0.0`; `../Rack-SDK/plugin.mk:6` derives VERSION via jq; tags `Low-frequency oscillator`/`Waveshaper` are canonical VCV spellings; slugs match `[A-Za-z0-9_-]+`; no trademarked synth/brand names in manifest fields (independently re-read) |
| 4 | `minRackVersion` 2.0.0 floor is safe — no Rack 2.1–2.6-only API is relied on (PKG-03 sub-claim) | UNCERTAIN — human verification requested | Code review (26-REVIEW.md WR-01) found the 26-01 SUMMARY validated only 2 of 6 version-sensitive APIs (`getLastFrameDuration`, `createIndexSubmenuItem`); `getScaledValue`, `isBypassed`, `addChildBelow`, `drawLayer` self-illumination were never checked against an actual 2.0.0 SDK. Only the 2.6-era `../Rack-SDK` is available locally — cannot resolve via grep |
| 5 | `make dist` produces a verified `.vcvplugin` (binary + populated plugin.json + res/ + LICENSE, no trial fonts) (PKG-05, roadmap SC3) | VERIFIED | Independently reran `rm -rf dist && make dist` (not trusting the SUMMARY's stale Jul-1 artifact) → produced `dist/ForgeAudio-AnalogSeries-2.0.0-mac-arm64.vcvplugin`; `tar --zstd -xf` extraction confirms `plugin.dylib`, `plugin.json` (populated URLs), full `res/` tree, `LICENSE`; `res/fonts/` contains only `JetBrainsMonoNL-Regular.ttf` + `OFL.txt` — no Barell/FoundationLogo |
| 6 | `M_PI` removed tree-wide, replaced with rack-free `forge::kPi`, bit-identical (TEST-06 / D-06 sub-fix) | VERIFIED | `grep -rn M_PI src/ tests/` → empty; `src/dsp/MathConst.hpp` declares `inline constexpr double kPi = 3.14159265358979323846;`, zero `#include` |
| 7 | Cross-platform golden restructured: drift-off leg (all OSes) + macOS-gated drift-on leg; `make test` green on macOS (TEST-06 / D-07 sub-fix) | VERIFIED (local) | Three `freerun_*_driftoff.f32` fixtures exist, each exactly 32768 bytes, git-tracked; `test_golden.cpp` shows `driftoff` TEST_CASEs outside `#if defined(__APPLE__)` and the bit-exact drift-on cases wrapped inside it; independently reran `make test` → **50 test cases / 2,615,027 assertions passed, 0 failed** (matches SUMMARY exactly) |
| 8 | The full test suite runs green as a GitHub Actions CI check on push (TEST-06, roadmap SC4 — the literal phase goal) | UNCERTAIN — human verification requested | `.github/workflows/test.yml` exists, correctly wired on `[push, pull_request]` across the 3-OS matrix (confirmed by reading the file). **However**, `git log origin/main..HEAD` shows the local branch is **25 commits ahead of origin/main** — none of Phase 26's fix commits have been pushed. `gh run list` shows the most recent actual CI run (`28934233470`, 2026-07-08) — which predates these fixes — as **FAILURE**. No CI run has ever executed against the current tree. Local macOS execution (truth #7) and code review support correctness, but the roadmap's literal wording ("runs green in CI") is a remote-system fact not yet observed |

**Score:** 5/7 verified (excluding the 1 legitimately deferred item); 2 items require human/remote action before the phase can be called fully closed.

### Deferred Items

| # | Item | Addressed In | Evidence |
|---|------|-------------|----------|
| 1 | plugin.json URL HTTP-200 reachability | Phase 28 | Phase 28 SC1 ("repo flipped to public..."); repo confirmed PRIVATE today; STATE.md documents the explicit Phase 25→28 sequencing constraint |

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `plugin.json` | Submission-ready manifest, populated URLs, minRackVersion 2.0.0 | VERIFIED | Re-read directly; matches both 26-01-SUMMARY claims and the packaged copy inside the `.vcvplugin` |
| `src/dsp/MathConst.hpp` | Rack-free `forge::kPi` constexpr | VERIFIED | Exists, zero includes, exact literal, wrapped in `namespace forge` |
| `tests/golden/freerun_{44100,48000,96000}_driftoff.f32` | 3 × 32768-byte cross-platform fixtures | VERIFIED | `wc -c` confirms exact size on all three; `git ls-files` confirms tracked |
| `tools/capture_golden.cpp` | Rack-free, spread-neutralized one-shot generator | VERIFIED (existence + content) | Contains `core.seed`, no `setSpreadSeed`; not picked up by the CI `tests/*.cpp` glob (lives in `tools/`, confirmed no `int main` collision) |
| `.github/workflows/test.yml` | 3-OS CI matrix wired on push | VERIFIED (wiring only) | Correct trigger (`on: [push, pull_request]`), correct matrix, correct Windows direct-g++ fallback with matching `TEST_CXXFLAGS`. Whether it currently reports green is unconfirmed (see Truth #8) |
| `dist/*.vcvplugin` | Verified packaged artifact | VERIFIED | Rebuilt from a clean `dist/` directory (not trusting the stale pre-existing artifact dated Jul 1, before URL population); listing and packaged `plugin.json` independently inspected |

### Key Link Verification

| From | To | Via | Status | Details |
|------|-----|-----|--------|---------|
| `plugin.json version` | `Makefile`/`plugin.mk` VERSION | `jq -r .version plugin.json` at `../Rack-SDK/plugin.mk:6` | WIRED | Confirmed no `VERSION` line in the project `Makefile`; `plugin.mk:6` performs the jq derivation |
| 5 consumer files (`Waveshape.hpp`, `DriftEngine.hpp`, `LfoCore.hpp`, `AnalogLFO.cpp`, `test_extraction.cpp`) | `src/dsp/MathConst.hpp` | `#include "dsp/MathConst.hpp"` + `forge::kPi` | WIRED | `grep -rn M_PI` empty confirms full substitution; build succeeds (both `make test` and `make dist`) |
| `tools/capture_golden.cpp` | `forge::LfoCore` | `core.seed(...)`, no `setSpreadSeed` | WIRED | Confirmed via grep; `make test` drift-off replay passes against the fixtures this generator produced |
| `plugin.json` (populated URLs) | packaged `.vcvplugin` | `make dist` → `rsync ... plugin.json dist/<slug>/` | WIRED | Extracted `plugin.json` from the freshly built artifact — URLs match the source manifest exactly |
| Fix commits (26-02, 26-03) | GitHub Actions `test.yml` run | `git push` → workflow trigger | **NOT WIRED (not yet exercised)** | 25 local commits unpushed; no CI run exists for the current tree — see Truth #8 |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full local test suite passes on macOS | `make test` | `50 test cases / 2,615,027 assertions passed, 0 failed` | PASS |
| M_PI fully removed | `grep -rn M_PI src/ tests/ tools/` | empty | PASS |
| `make dist` produces a valid, inspectable `.vcvplugin` | `rm -rf dist && make dist && tar --zstd -tvf ...` | Full listing confirmed: binary, plugin.json, res/, LICENSE, NOTICES, no trial fonts | PASS |
| Packaged manifest URLs match source | `tar --zstd -xf ... && cat plugin.json` | All three URLs = `https://github.com/Photep/ForgeAudio-AnalogSeries` | PASS |
| Remote repo visibility matches deferral claim | `gh repo view --json visibility`, `curl -sSI <url>` | `PRIVATE`, HTTP 404 | PASS (confirms the deferral is real, not a cover story) |
| GitHub Actions has run against current fixes | `gh run list`, `git log origin/main..HEAD` | 25 commits unpushed; latest real run predates fixes and is FAILURE | **FAIL — see Truth #8 / human verification** |

### Probe Execution

No `scripts/*/tests/probe-*.sh` convention or PLAN/SUMMARY-declared probes found for this phase. SKIPPED (no formal probes; behavioral spot-checks above cover the equivalent ground).

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|--------------|--------|----------|
| PKG-02 | 26-01 | `plugin.json` `authorUrl`/`pluginUrl`/`sourceUrl` populated | SATISFIED (populate half) / DEFERRED (reachability half, Phase 28) | See Truths 1–2 |
| PKG-03 | 26-01 | Manifest validated for submission | SATISFIED, with one open verification gap (WR-01, minRackVersion API audit) | See Truths 3–4 |
| PKG-05 | 26-04 | `make dist` produces a verified `.vcvplugin` | SATISFIED | See Truth 5 |
| TEST-06 | 26-02, 26-03 | Full test suite runs green, wired as CI check on push | Wiring SATISFIED; "runs green in CI" NEEDS HUMAN (push + observe) | See Truths 6–8 |

**Traceability cross-check against `.planning/REQUIREMENTS.md`:** All four requirement IDs (PKG-02, PKG-03, PKG-05, TEST-06) are present in the requirements list and correctly mapped to Phase 26 in the traceability table — no orphaned or unmapped IDs. **Gap noted (process, non-blocking):** the `- [ ]` checkboxes for these four requirements in `.planning/REQUIREMENTS.md` (and the "Pending" status in the traceability table) were never flipped by any of the four `docs(26-0N): complete ... plan` commits, unlike the pattern established in prior phases (e.g. `5a895ec docs(24-03)` updated `REQUIREMENTS.md`/`ROADMAP.md`/`STATE.md` together; Phase 26's plan-completion commits touched only the `-SUMMARY.md` files, with `ROADMAP.md`/`STATE.md` updated separately in `f974fda`/`cf6718c`). `REQUIREMENTS.md` itself was never touched. This does not affect functional goal achievement but should be corrected before milestone audit.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| — | — | No `TBD`/`FIXME`/`XXX`/`TODO`/`HACK`/`PLACEHOLDER` markers found in any of the 9 phase-touched files | — | Clean |
| `.planning/REQUIREMENTS.md` | 19,39,40,42 | Checkboxes for TEST-06/PKG-02/PKG-03/PKG-05 still `[ ]` despite phase completion | ℹ️ Info | Process/bookkeeping gap, not a functional defect — see Requirements Coverage note |
| `plugin.json` / `tests/test_golden.cpp` | `minRackVersion:5` / header comment | WR-01 (API-surface audit incomplete) / WR-02 (epsilon rationale cites wrong ops, per 26-REVIEW.md) | ⚠️ Warning (carried from code review) | WR-01 promoted to a human-verification item above (real compliance risk); WR-02 (documentation-only, doesn't affect correctness) noted but not re-escalated — cosmetic doc-accuracy issue in a code comment, does not change test behavior |

### Human Verification Required

### 1. Push and confirm GitHub Actions CI is actually green

**Test:** Push the 25 unpushed local commits to `origin/main` and watch the `test` workflow run (`gh run watch` or the Actions UI) across all three OS legs.
**Expected:** ubuntu-latest, macos-latest, and windows-latest all report SUCCESS, with the same 50/50 test-case pass rate confirmed locally.
**Why human:** This is the literal, unambiguous wording of both the phase goal and roadmap Success Criterion 4 ("runs green in CI"). No push has happened since the fix commits landed; the last real remote run predates the fix and failed. This is a remote-system fact, not something resolvable by static analysis, and pushing to `origin/main` is outside verifier scope.

### 2. Audit the minRackVersion=2.0.0 API-surface claim

**Test:** Compile (or otherwise confirm via Rack changelog/git history) that `ParamQuantity::getScaledValue()`, `Module::isBypassed()`, `Widget::addChildBelow()`, and the `drawLayer` self-illumination pattern used in `src/AnalogLFO.cpp` all exist and behave correctly under the actual Rack 2.0.0 SDK (not just the 2.6-era SDK currently vendored at `../Rack-SDK`).
**Expected:** All six version-sensitive APIs resolve cleanly against Rack 2.0.0, validating the `minRackVersion: "2.0.0"` floor — or the floor is raised to whatever version is actually confirmed.
**Why human:** Flagged by code review (26-REVIEW.md WR-01) as an incomplete audit — the 26-01 plan only checked 2 of 6 relevant symbols. Resolving requires either downloading a Rack 2.0.0 SDK or manual changelog archaeology; not achievable via local grep in this environment.

### Gaps Summary

No hard functional gaps were found in the codebase itself: the manifest is field-valid, the `.vcvplugin` artifact independently rebuilds and verifies clean, the M_PI portability fix is complete and bit-exact, and the golden restructure is correctly gated and passes 50/50 locally on macOS. The phase's engineering work is sound.

What remains open is precisely the phase's own headline claim — "the full test suite runs green in CI" — which has not actually been observed on the remote system, because the fix commits were never pushed. This is a procedural/verification gap, not a code defect, but it is the literal roadmap Success Criterion 4 and cannot be marked VERIFIED on local evidence alone. A secondary, lower-severity item (the minRackVersion 2.0.0 API-surface audit) was flagged by code review and remains genuinely unverified. The PKG-02 URL-reachability sub-item is correctly and legitimately deferred to Phase 28 per explicit, corroborated project sequencing — not counted as a gap.

---

*Verified: 2026-07-09T02:36:01Z*
*Verifier: Claude (gsd-verifier)*
