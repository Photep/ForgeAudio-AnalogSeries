# Project Research Summary

**Project:** Forge Audio — Analog Series (v1.4 "Tempered")
**Domain:** VCV Rack 2 plugin release hardening — automated testing, IP compliance, VCV Library submission, Notion user manual
**Researched:** 2026-06-14
**Confidence:** HIGH

---

## Executive Summary

v1.4 "Tempered" is a release-hardening milestone for a feature-frozen VCV Rack 2 LFO. No new DSP features ship. The work divides into four tracks with strict ordering constraints between them: (1) extract pure DSP into a Rack-independent header-only core and build a doctest-driven test harness; (2) fix four functional bugs from CODE-REVIEW-FINDINGS.md, each landing as a regression test; (3) harden the repo for public release — purge trial fonts from git history while the GitHub repo is still private, add a GPL LICENSE file, populate manifest URLs, create a CHANGELOG, and audit assets for IP compliance; (4) write and publish the Notion user manual, then flip the repo public, and finally open the VCV Library submission issue.

The load-bearing ordering constraints are: the test harness must precede DSP refactors (findings #11, #12), and the git-history purge must complete while the repo is still private before the repo is ever made public. These two constraints are the only hard dependencies across the four tracks; everything else is sequenced for efficiency. The VCV Library submission mechanism is a GitHub issue on VCVRack/library titled with the plugin slug (`ForgeAudio-AnalogSeries`), providing a source URL and exact commit hash — not a PR, not a branch name. The manifest `version 2.0.0` is correct and must not be changed to `1.4.x`.

The primary risks are: (a) flipping the GitHub repo public before the font-purge is verified against a fresh remote clone — a non-recoverable IP leak once public; (b) performing the #11/#12 display/thread refactors before the test harness exists, leaving no proof of output preservation; (c) adopting the #2 ratio-alignment table change without a documented listening decision, shipping a groove regression that automated tests cannot detect. All three risks have concrete prevention procedures documented in PITFALLS.md.

---

## Key Findings

### Recommended Stack

The existing build system (`plugin.mk`, no external dependencies, C++17) requires no changes. The one new tooling decision is the C++ test framework: **doctest**, vendored as the single amalgamated header `tests/doctest/doctest.h` (pin v2.4.11 or v2.5.2). Doctest wins over Catch2 v3 because Catch2 v3 dropped its single-header distribution and now requires CMake — which fights a `plugin.mk` Makefile. Doctest compiles in ~10 ms per translation unit vs Catch2's ~430 ms, requires zero build-system changes, and has identical API ergonomics (`TEST_CASE`, `CHECK`, `REQUIRE`). GoogleTest and Boost.Test are excluded for the same build-friction reasons.

`make test` is a purely additive target appended after `include $(RACK_DIR)/plugin.mk`. It compiles only `src/dsp/*.hpp` (header-only, zero Rack includes) and `tests/*.cpp`, linking nothing from the Rack SDK. The plugin build path is unchanged.

**Core technologies:**
- **VCV Rack 2 SDK + plugin.mk:** unchanged — existing build system is canonical
- **doctest v2.4.11+:** header-only test framework; drop `doctest.h` into `tests/`; no CMake, no submodules
- **git-filter-repo:** the tool for font history purge (officially recommended; auto-removes `origin` as a guardrail; use over BFG for path-precision and auditability)
- **GitHub Issues (VCVRack/library):** the submission mechanism — one issue per plugin, title = slug, body = sourceUrl + commit hash + license

**VCV manifest facts (do not get these wrong):**
- `version 2.0.0` is CORRECT (MAJOR must equal Rack major = 2). Do not "fix" to `1.4.x`.
- `slug ForgeAudio-AnalogSeries` and module slug `ForgeAnalogLFO` are permanent once published — freeze now.
- `GPL-3.0-or-later` is VCV's recommended license for open-source plugins — no change needed.
- `sourceUrl` must be the GitHub project page (`https://github.com/Photep/ForgeAudio-AnalogSeries`), not the `.git` clone URL.
- Submission is a GitHub **Issue** titled with the slug, not a PR. Post `git rev-parse HEAD`, not a branch name.
- `minRackVersion: 2.6.0` should be reconsidered — it excludes Rack 2.0–2.5 users unless a 2.6 API is actually required.

### Expected Features (Manual Sections)

This is a documentation milestone. "Features" = manual sections.

**Table stakes (a VCV manual is incomplete without these):**
- Overview / concept with hero panel image
- Annotated panel screenshot with numbered callouts
- Control reference TABLE (knobs + buttons) — the spine of the manual and the differentiator vs every surveyed VCV manual (none use a clean table)
- Inputs / outputs reference with voltage ranges and polarity
- Right-click / context-menu section (invisible on panel; universally expected in VCV docs)
- Rate dual-mode explanation (free Hz vs 15 musical ratios — genuinely confusing without docs)
- Clock sync explanation (3-state tracker, SYNC badge states, phase reset, display pills)
- Installation: VCV Library path AND manual `.vcvplugin` path — both required
- Changelog (liftable directly from MILESTONES.md)
- Credits / License (GPL-3.0, source repo link)

**Should have (differentiators for this module's 3-axis engine):**
- "Understanding the Analog Engine" concept page — the Morph/Character/Drift mental model; one subsection per axis; highest-leverage page for adoption
- The Display deep-dive (three-column CRT layout, animated SYNC flash, phase dot)
- 2–4 patch/usage examples (slow filter sweep, tempo-synced wobble, living vibrato, self-FM)
- Animated GIFs of drift and SYNC flash (Stoermelder convention for time-based behavior)
- Signal-flow diagram (Mutable/Befaco tradition for analog-modeled modules)
- Troubleshooting / FAQ (pre-empts clock sync questions; reference resolved v1.4 fixes as closed)

**Cut / avoid:**
- DSP internals (OU layers, EMA alpha, crossfade math) — code comments, not user manual
- Electrical specs — meaningless in software
- VCO / future module pages — out of this milestone's scope
- Trademark-y "sounds like a Minimoog" claims — legal/trademark risk

**Notion structure:** 12 subpages with `/Table of contents` block on top-level page and each long subpage; Notion simple tables (not databases) for control/jack references; "Share to web" on, search-engine indexing on, editing/comments off.

**Authoring order:** Write sections 1, 4, 5, 8, 10, 12 first (table-stakes spine — shippable on its own); then 2, 6, 7, 9 (differentiators); then 3, 11 (polish). Changelog comes from MILESTONES.md directly; CV voltage ranges verified against `AnalogLFO.cpp` before publishing.

### Architecture Approach

Extract all DSP logic from `struct AnalogLFO : Module` into header-only, Rack-independent units under `src/dsp/*.hpp`. The `AnalogLFO.cpp` Rack shell becomes a thin pass-through: unpack `params[]`/`inputs[]`/`ProcessArgs` into a `forge::Inputs` POD, call `core.step(in)`, write `outputs[]` and display atomics. `process()` shrinks to ~20 lines. This is the unanimous pattern in VCV plugins with test suites (Squinky Labs, Surge XT).

Do NOT link `libRack` into the test binary. The SDK ships `libRack.dylib` (12 MB) and technically exports engine symbols, but it drags in `Window`/GL/`APP` globals — bare `process()` calls without a configured `Engine` dereference uninitialized globals. Linking is unsupported, platform-fragile, and buys nothing that the extracted-core harness does not provide.

**Major components:**
1. **`src/dsp/RackCompat.hpp`** — forge-owned shims: `SchmittTrigger`, `Timer`, `ExponentialFilter` (~10 lines each); replace `rack::math::clamp` with `std::clamp`
2. **`src/dsp/ClockTracker.hpp`** — EMA + consecutive-outlier counter + 3-state FSM; `step(float clkVoltage, float dt)` interface; home of the #1 fix
3. **`src/dsp/RatioTable.hpp`** — 15-ratio table + `BEATS_PER_ALIGN[15]` + `shouldReset(ratioIdx, beatCount)`; pure integer math; home of the #2 fix; easiest high-value unit-test target
4. **`src/dsp/Waveshape.hpp`** — `computeSine/Tri/Saw/Square/Pulse/Morph`; bleed dependency (`ouLayers[0].state`) becomes an explicit `float bleedLfo` parameter
5. **`src/dsp/Swing.hpp`** — `swingPhaseMultiplier(phase, swingFrac, isClocked)` and display warp; home of the #3 fix
6. **`src/dsp/DriftEngine.hpp`** — OU layers + jitter + DC; templated on RNG type for deterministic test injection
7. **`src/dsp/LfoCore.hpp`** — orchestrator taking `forge::Inputs` POD, returning output voltage
8. **`tests/BlockDriver.hpp`** — owns `LfoCore` + fixed sample rate; synthesizes input blocks; IS the headless integration harness
9. **`tests/test_main.cpp` + `tests/test_*.cpp`** — doctest runner + per-finding regression tests (#1 outlier recovery, #2 ratio alignment, #3 swing desync, #4 JSON crash guard) + invariant tests (output bounds, frequency accuracy, phase continuity, morph/character ranges, determinism)

### Critical Pitfalls

1. **Removing trial fonts from HEAD only, skipping history rewrite** — fonts are already pushed to the private remote in commit `e486ce1`. Required: `git filter-repo --path BCBarellTEST-Regular.otf --path FoundationLogo.ttf --invert-paths` on a fresh `--no-local` clone, `git push --force --all --tags origin`, then verify with a four-step grep on a fresh re-clone of the remote. Do this while the repo is still PRIVATE.

2. **Flipping the repo public before purge is verified against a fresh remote clone** — `push --force origin main` only rewrites one ref; stale branches/tags still contain the old commits. Force-push with `--all --tags`. The publish step is a separate phase gated on `git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'` returning empty on a fresh re-clone.

3. **Performing the #11/#12 display/thread refactors before the test harness exists** — moving `updateDisplayBuffer()` off the audio thread introduces a thread boundary; replacing fixed `1/60` ticks with wall-clock `dt` changes all animation math. Without a pinned headless harness there is no proof the refactor preserved behavior. Harness first; refactors gated on harness-green.

4. **Adopting the #2 ratio-alignment table without a documented listening decision** — the current x1.5 every-beat retrigger may be a desirable groove. Build both versions, audition in Rack, record the decision (with audio if possible), then write the regression test that pins the *chosen* behavior. A test encoding the math without anyone having listened is insufficient.

5. **Setting `version` to `1.4.x` in plugin.json** — the internal milestone label "v1.4 Tempered" and the manifest `version` are different numbering schemes. Manifest MAJOR must equal Rack major = 2. `2.0.0` is correct. Changing it to `1.x` breaks VCV's updater and the submission.

---

## Implications for Roadmap

Two hard ordering constraints govern all phases:
- **Constraint A:** Test harness (Phase 1) must precede all DSP refactors (Phase 3).
- **Constraint B:** Git-history purge (Phase 4) must complete while repo is PRIVATE and be verified via fresh remote clone before the repo is flipped public (Phase 6).

Everything else is relatively independent within its phase.

---

### Phase 1: Test Harness Foundation

**Rationale:** Load-bearing prerequisite for safe DSP refactors (#11, #12) and for bug fixes landing as regression tests (PROJECT.md mandate). Build it before any other code changes.
**Delivers:**
- `src/dsp/` directory with `RackCompat.hpp` shims
- `tests/doctest/doctest.h` vendored (pinned v2.4.11 or v2.5.2)
- `tests/test_main.cpp` defining the doctest runner
- Makefile `test` target (additive; does not touch plugin build; `RACK_DIR` irrelevant to it)
- `tests/BlockDriver.hpp` headless integration harness over `LfoCore`
- Stub/passing tests for output-bounds, frequency-accuracy, morph/character-range invariants
**Avoids:** DSP refactor regressions with no safety net (Pitfall 6); tested code drifting from shipped code (Architecture anti-pattern)
**Research flag:** No deeper research needed — architecture is well-documented and doctest drop-in is trivial.

---

### Phase 2: Functional Bug Fixes

**Rationale:** Each fix lands with a regression test going from failing to passing. The harness from Phase 1 enables this. Finding #2 has a hard audition gate that cannot be skipped.
**Delivers:**
- Fix #3 (phase dot desync): store `isClocked ? swingFrac : 0.5f` as `displaySwingFraction` — one line; regression test added
- Fix #4 (malformed-JSON crash): `json_is_string()` guard + try/catch fallback; regression test on parsing predicate as free function
- Fix #1 (clock tracker lockout): consecutive-outlier counter in `ClockTracker`; regression test asserts re-acquisition after >3× tempo jump and after fast-to-slow trap
- Fix #2 (x1.5/÷1.5 beat alignment): **AUDITION-GATED** — build both versions, listen in Rack, record decision, then add `BEATS_PER_ALIGN` table to `RatioTable.hpp` and regression test that pins chosen behavior
**Avoids:** #2 groove regression shipped without human listening decision (Pitfall 7); missing regression tests (PROJECT.md mandate)

---

### Phase 3: DSP Extraction and Display Refactors

**Rationale:** Gated on Phase 1 (harness green). This is where #11 and #12 land alongside full `src/dsp/` extraction.
**Delivers:**
- Full `src/dsp/` layer: `Waveshape.hpp`, `Swing.hpp`, `DriftEngine.hpp`, `LfoCore.hpp`
- `AnalogLFO.cpp` shell thinned to ~20-line `process()` pass-through
- Fix #11 (frame-rate-independent animation): `APP->window->getLastFrameDuration()` with `clamp(dt, 0, 1/30.f)` ceiling
- Fix #12 (display buffer off audio thread): move `updateDisplayBuffer()` to `WaveformDisplay::step()` behind existing atomics + double-buffer pattern
- Code cleanups: #8 (dead `drawZeroCrossing` + `scanlineImage`), #9 (unreachable `isStill`), #10 (pill fade symmetry on clock disconnect)
- Headless harness output verified within epsilon pre/post at 44.1/48/96 kHz
**Avoids:** Silent regression from thread-boundary introduction (Pitfall 6); bleed-modulation parameter lift missed during extraction (Architecture gap — flag on extraction checklist)
**Research flag:** One decision to confirm at phase start: RNG strategy (re-implement Xoroshiro128+ in core for bit-identical fidelity vs. template on RNG and inject `std::mt19937_64` in tests). Recommendation: re-implement Xoroshiro128+ for fidelity.

---

### Phase 4: Release IP Hardening

**Rationale:** Must execute while the GitHub repo is still PRIVATE. Rewrites every commit SHA from `e486ce1` onward; must be its own dedicated phase on a quiet tree with nothing else in flight.
**Delivers:**
- `BCBarellTEST-Regular.otf` and `FoundationLogo.ttf` purged from all history via `git filter-repo --invert-paths` on a fresh clone, force-pushed with `--all --tags`
- Purge verified: `git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'` returns empty on a fresh clone of the remote (not just the local repo)
- `LICENSE` file at repo root (GPLv3 full text)
- `NOTICES` or `res/fonts/README` with OFL attribution for JetBrains Mono and any other shipped font
- Asset inventory for `res/`: confirm Bebas Neue / Chakra Petch outlines in panel SVG came from OFL Google Fonts cuts, not trial/commercial cuts
- `.gitignore` entry blocking future non-OFL font commits
**Avoids:** Trial fonts redistributed via public history (Pitfalls 1, 2, 3 — non-recoverable once public)
**Research flag:** SVG font-outline provenance is an open question — resolve by inspection at phase start (see Open Questions).

---

### Phase 5: VCV Library Compliance

**Rationale:** All code work and IP hardening are complete. This is the manifest/packaging/verification pass before publication.
**Delivers:**
- `plugin.json` URLs populated: `sourceUrl` (GitHub project page), `authorUrl`, `pluginUrl`, `manualUrl` (Notion URL — requires Phase 6a Notion publish first; this field can be filled at end of Phase 5 or start of Phase 7)
- `plugin.json version` confirmed at `2.x.x`; Makefile `VERSION` confirmed identical
- `minRackVersion` decision documented (lower from `2.6.0` to `2.0.0` unless a 2.6 API is confirmed required)
- `CHANGELOG.md` created (lifted from MILESTONES.md); `changelogUrl` added to manifest
- Tag audit: consider adding `Random` tag (OU drift engine); confirm no invalid tags exist
- SVG lint: `res/AnalogLFO.svg` contains no `<filter>`, `feGaussianBlur`, CSS, or live `<text>` nodes (nanosvg subset constraint)
- Trademark string audit: no trademarked synth names in manifest fields, panel text, preset names
- `make dist` run; `.vcvplugin` artifact unzipped and inspected: `LICENSE` present, `plugin.json` URLs populated, `res/` present, no trial fonts
- Clean build verified on macOS; GitHub Actions CI matrix (ubuntu/macos/windows) added if feasible
**Avoids:** Manifest rejection (Pitfall 5); version confusion (Pitfall 8); trademark triggers (Pitfall 4)

---

### Phase 6: Publish Repo and Manual

**Rationale:** Repo goes public only after Phase 4 purge is verified. Manual drafting can start during any earlier phase; must be published before Phase 7.
**Delivers:**
- 6a: Notion manual published to web (12 subpages; table-stakes sections 1/4/5/8/10/12 first, then 2/6/7/9, then 3/11)
- 6b: GitHub repo flipped public (`gh repo edit --visibility public`) — gated on Phase 4 verification
- `plugin.json` `sourceUrl`, `pluginUrl`, `manualUrl` verified as live URLs and committed
**Avoids:** Public repo with contaminated history (Pitfalls 1/2 — Phase 4 purge verification is the gate)

---

### Phase 7: VCV Library Submission

**Rationale:** Final phase. All prerequisites complete: code clean, tests green, IP hardened, repo public, manifest valid, manual published.
**Delivers:**
- Git tag `v2.x.y` at release commit
- `git rev-parse HEAD` commit hash recorded
- One issue opened at https://github.com/VCVRack/library/issues titled `ForgeAudio-AnalogSeries` (the slug, not the name)
- Issue body: plugin name, license (`GPL-3.0-or-later`), sourceUrl, pluginUrl, authorUrl, manualUrl, exact commit hash (NOT a branch name)
- Submission issue URL recorded for all future update comments (this thread is permanent)
**Avoids:** Issue titled with plugin name (Pitfall 5); branch name instead of commit hash (Pitfall 5)
**Research flag:** No deeper research needed — submission procedure is fully documented.

---

### Phase Ordering Rationale

- Phase 1 before Phases 2 and 3: harness must exist before any code is expected to carry regression tests, and before refactors that need a behavioral pin.
- Phase 2 before Phase 3: bugs fixed first; refactors do not introduce new variables while bugs are outstanding.
- Phase 4 as early as feasible (can run after Phase 2, before or concurrent with Phase 3): purge rewrites history; doing it earlier reduces SHA-rebase surface area. Must happen on a quiet tree.
- Phase 5 after Phase 3: compliance pass needs the final code state.
- Phase 6b (public flip) gated on Phase 4 purge verification: hard constraint.
- Phase 6a (Notion publish) can start drafting during any earlier phase; must publish before Phase 7.
- Phase 7 last: all gates must be green.

### Research Flags

**No additional research needed (patterns fully established):**
- Phase 1 (Test Harness): doctest drop-in, Makefile additive-target pattern — fully documented in STACK.md and ARCHITECTURE.md
- Phase 2 (Bug Fixes): all four fixes concretely specified in CODE-REVIEW-FINDINGS.md; #2 has an audition gate, not a research gate
- Phase 4 (IP Hardening): git-filter-repo procedure fully documented; only SVG provenance needs inspection at phase start
- Phase 7 (Submission): VCV issue-based flow fully documented

**One open question to resolve at phase start:**
- Phase 3 (DSP Extraction): RNG strategy decision — low-stakes, decide at phase start
- Phase 5 (Compliance): `minRackVersion` decision requires a one-time API grep

---

## Open Questions

| Question | Where to Resolve | Stakes |
|----------|-----------------|--------|
| **SVG font provenance:** do Bebas Neue / Chakra Petch outlines baked into `res/AnalogLFO.svg` come from the OFL Google Fonts cuts or from a trial/commercial cut? | Phase 4 — inspect SVG source and font file provenance records | HIGH — non-OFL outlines in the production SVG is a redistribution issue; OFL just needs NOTICES attribution |
| **"Forge" brand collision check:** does slug `ForgeAudio-AnalogSeries` or brand "Forge Audio" collide with any existing VCV Library plugin or trademark? | Phase 5 — search VCV Library and library issue tracker for "Forge" | MEDIUM — slug is permanent; confirm before submission |
| **Author/source URLs:** are `authorUrl` and `pluginUrl` the Photep GitHub org page and repo page, or a separate Forge Audio domain? | Phase 5 / Phase 6 — decide and record in manifest | LOW — informational; does not block submission but should be deliberate |
| **`minRackVersion` decision:** is `2.6.0` actually required by any API, or can it be lowered to `2.0.0` to widen audience? | Phase 5 — grep `AnalogLFO.cpp` for API calls introduced after Rack 2.0 | MEDIUM — free audience expansion if lowerable |
| **NOTICES file scope:** which assets beyond JetBrains Mono need explicit OFL attribution? Any other binaries in `res/`? | Phase 4 — asset inventory | MEDIUM — required for clean GPL compliance |
| **#2 audition decision:** is the current x1.5 every-beat retrigger a desirable groove or an artifact? | Phase 2 — explicit listening session before implementing `BEATS_PER_ALIGN` table | HIGH for musical quality — cannot be delegated to automated tests |

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack (VCV manifest facts, submission mechanism, doctest) | HIGH | All facts verified against official VCV manual, VCVRack/library README, and doctest GitHub. No ambiguity on submission flow or framework choice. |
| Features (manual sections, Notion structure) | HIGH | Cross-referenced against 5 leading VCV author manuals + official VCV docs + official Notion help docs. Table-stakes vs. nice-to-have split is well-supported. |
| Architecture (extract-core approach, test harness design) | HIGH | Corroborated by Squinky Labs and Surge XT (two major VCV plugins with test suites), VCV community thread consensus, and direct SDK inspection. Decoupling surface assessed line-by-line from the 1,641-line source. |
| Pitfalls (git-history purge procedure, manifest rejection causes) | HIGH | Repo state verified directly this session via `git log`, `git ls-files`, `gh repo view` (PRIVATE, pushed, fonts in e486ce1). VCV manifest and submission rules verified against official sources. Purge procedure verified against git-filter-repo docs. |

**Overall confidence:** HIGH

### Gaps to Address

- **SVG font-outline provenance** (Phase 4): inspect `res/AnalogLFO.svg` for outlined glyphs from Bebas Neue / Chakra Petch and confirm which font files generated them.
- **`minRackVersion` API audit** (Phase 5): grep source for any `rack::` API call requiring Rack 2.6+; if none found, lower to `2.0.0`.
- **RNG strategy** (Phase 3, phase start): decide whether to re-implement Xoroshiro128+ in the core (bit-identical drift with shipped behavior) or template `DriftEngine` on an RNG type. Recommendation: re-implement Xoroshiro128+ for fidelity.
- **`computeMorphedWave` bleed dependency** (Phase 3, extraction checklist): `ouLayers[0].state` is currently read inside `computeMorphedWave` for bleed modulation — must become an explicit `float bleedLfo` parameter during extraction. Easy to miss; add to the extraction checklist.
- **"Forge" collision check** (Phase 5): search VCV Library and issue tracker before submitting; slug is permanent.

---

## Sources

### Primary (HIGH confidence)
- https://vcvrack.com/manual/Manifest — manifest field list, slug rules, version format, tag vocabulary, minRackVersion
- https://vcvrack.com/manual/PluginLicensing — GPL-3.0-or-later acceptance, ethics/trademark rules, asset licensing
- https://vcvrack.com/manual/Building — `make dist`, `.vcvplugin` contents, DISTRIBUTABLES
- https://github.com/VCVRack/library (README) — Issue-based submission, slug-title rule, commit-hash rule, no-branch-name rule
- https://vcvrack.com/manual/Installing — both install paths (Library + manual .vcvplugin)
- https://github.com/doctest/doctest — single-header `doctest.h`, v2.4.11/v2.5.2, C++11–23
- https://github.com/newren/git-filter-repo — `--path`/`--invert-paths`, fresh-clone guardrail, force-push requirement
- Direct repo inspection this session: `git log`, `git ls-files`, `gh repo view` (PRIVATE, pushed, fonts in e486ce1), `plugin.json` state, `src/AnalogLFO.cpp` line-by-line coupling analysis, `../Rack-SDK/libRack.dylib` inspection

### Secondary (MEDIUM confidence)
- https://accu.org/journals/overload/25/137/kirilov_2343/ and https://hackingcpp.com/cpp/tools/testing_frameworks — doctest ~10ms vs Catch ~430ms compile overhead; Catch2 v3 dropped single-header
- https://community.vcvrack.com/t/unit-testing/7477 — Squinky Labs "extract the core" testimony; community consensus on DSP isolation
- https://community.vcvrack.com/t/tip-rack-sdk-with-catch2-via-vcpkg-and-cmake-with-ci-cd/23244 — CI friction when linking Rack; reinforces headers-only path
- Surge XT for Rack — Catch2 test suite over Rack-independent engine (structure reference)
- Stoermelder PackOne docs — per-feature sections, appended changelog, GIFs for time-based behavior (manual structure reference)
- Bogaudio Modules README — standardized polyphony/bypass footer convention
- Notion help docs — Wiki, ToC block, callout, table, Share to web, publishing settings
- https://openfontlicense.org/ — OFL redistribution terms for JetBrains Mono, Bebas Neue, Chakra Petch
- git-tower.com, coreui.io, marcofranssen.nl — corroborating git-filter-repo flags and force-push procedure

---
*Research completed: 2026-06-14*
*Ready for roadmap: yes*
