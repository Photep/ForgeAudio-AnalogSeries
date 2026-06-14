# Stack Research — Release & Packaging for VCV Library Submission

**Domain:** VCV Rack 2 plugin publishing / release hardening (v1.4 "Tempered")
**Researched:** 2026-06-14
**Confidence:** HIGH (VCV facts verified against vcvrack.com/manual and VCVRack/library README; test-framework comparison verified against doctest GitHub + ACCU/hackingcpp)

---

## Scope

This is a release-only milestone. The audio engine, build system (`plugin.mk`, no external deps), and panel are frozen. This document covers exactly what is needed to **publish** the source and **submit** the existing plugin to the VCV Library, plus the one new build-tooling decision the milestone introduces: a C++ test target.

Two practical surprises up front, both verified:

1. **VCV Library submission is via a GitHub *Issue*, not a PR.** You do not open a pull request against `VCVRack/library`. You open one issue per plugin, titled with your **plugin slug**, and post your metadata + source URL + commit hash in it. (The old `manifests/*.json` PR flow is legacy; the current README directs submitters to the Issue Tracker. The library build infrastructure that compiles your source for all platforms is operated by VCV, not driven by a file in your repo.)
2. **Your plugin slug `ForgeAudio-AnalogSeries` is locked forever once published.** It is the permanent identity and the issue title. Confirm it is what you want *before* the first submission — it can never change.

---

## Part 1 — `plugin.json` Manifest: Required vs Optional

Verified against https://vcvrack.com/manual/Manifest.

### Plugin-level fields

| Field | Required? | Rule / Format | Your current value | Action for v1.4 |
|-------|-----------|---------------|--------------------|-----------------|
| `slug` | **REQUIRED** | `[a-zA-Z0-9_-]` only. Permanent — never change after release. | `ForgeAudio-AnalogSeries` | Keep (verify it's final) |
| `name` | **REQUIRED** | Human-readable plugin name | `Forge Audio - Analog Series` | Keep |
| `version` | **REQUIRED** | `MAJOR.MINOR.REVISION`, **no `v` prefix**. MAJOR must match Rack major (`2`). | `2.0.0` | Keep `2.x.x`; bump on each Library build (see Part 3) |
| `license` | **REQUIRED** | SPDX id, `"proprietary"`, or EULA URL | `GPL-3.0-or-later` | Keep — **confirmed accepted** (see Part 2) |
| `author` | **REQUIRED** | Name/company/alias/GitHub user | `Forge Audio` | Keep |
| `brand` | optional | Prefix prepended to all module names in browser | `Forge Audio` | Keep |
| `description` | optional | One-line summary | present | Keep |
| `authorUrl` | optional | Author/company homepage | `""` | **Fill** (finding #6) |
| `pluginUrl` | optional | Plugin homepage | `""` | **Fill** (finding #6) |
| `sourceUrl` | optional* | Source repo **project page** (NOT the `.git` URL) | `""` | **Fill — de-facto required** (finding #6) |
| `manualUrl` | optional | HTML/PDF/GitHub manual | absent | **Add** — Notion manual URL (milestone deliverable) |
| `authorEmail` | optional | Support contact (public if set) | absent | Optional |
| `donateUrl` | optional | Donation link | absent | Skip |
| `changelogUrl` | optional | Version history | absent | Optional (CHANGELOG.md on GitHub) |
| `minRackVersion` | optional | Min Rack version; enforced for `2.4.0+` | `2.6.0` | Keep (or lower to `2.0.0`/`2.4.0` to widen audience — see note) |

\* `sourceUrl` is formally optional in the manifest schema, but the **submission requires you to provide the source URL** in the issue, and open-source Library distribution builds *from* that source. Treat `sourceUrl` as required for your case. It must be the **GitHub project page** (`https://github.com/<user>/<repo>`), not the clone URL ending in `.git`.

> **`minRackVersion` note:** `2.6.0` excludes everyone on 2.0–2.5. Unless the code genuinely depends on a 2.6 API, lower it (e.g. `2.0.0`) to maximize reach. This is a free decision to revisit during requirements.

### Module-level fields (each entry in `modules[]`)

| Field | Required? | Rule | Your value | Action |
|-------|-----------|------|------------|--------|
| `slug` | **REQUIRED** | Slug charset; **permanent** | `ForgeAnalogLFO` | Keep (locked once shipped) |
| `name` | **REQUIRED** | Human-readable; `brand` is auto-prefixed in browser | `Analog LFO` | Keep |
| `description` | optional | Browser tooltip one-liner | present | Keep |
| `tags` | optional | From VCV's **fixed tag vocabulary**, case-insensitive | `["Low-frequency oscillator","Waveshaper"]` | Valid — see below |
| `keywords` | optional | Hidden search aliases | absent | Optional (e.g. `"LFO drift analog morph"`) |
| `manualUrl` | optional | Module-specific manual; falls back to plugin `manualUrl` | absent | Skip (plugin-level covers it) |
| `modularGridUrl` | optional | For hardware clones only | absent | Skip (original design) |
| `hidden` | optional | Boolean; hide deprecated modules | absent | Skip |

**Tag validation:** tags must come from VCV's predefined list (Arpeggiator, Attenuator, Blank, Chorus, Clock generator, Compressor, Controller, Delay, Digital, Distortion, Drum, Dual, Effect, Envelope follower, Envelope generator, Equalizer, Expander, External, Filter, Flanger, Function generator, Granular, Hardware clone, Limiter, Logic, Low-frequency oscillator, Low-pass gate, MIDI, Mixer, Multiple, Noise, Oscillator, Panning, Phaser, Physical modeling, Polyphonic, Quantizer, Random, Recording, Reverb, Ring modulator, Sample and hold, Sampler, Sequencer, Slew limiter, Switch, Synth voice, Tuner, Utility, Visual, Voltage-controlled amplifier, Waveshaper, …). Your two are valid. Consider adding **`Random`** (the drift/OU engine is a defining feature). Do **not** add `Polyphonic` — the module is monophonic by design.

> An invalid tag is the single most common manifest rejection. If you add tags, copy them character-exact from the manual; the matcher is case-insensitive but vocabulary-strict.

---

## Part 2 — Licensing: Whitelist & GPL Compatibility

Verified against https://vcvrack.com/manual/PluginLicensing and community confirmation.

- **`GPL-3.0-or-later` is accepted and is VCV's *recommended* license** for open-source plugins. VCV explicitly: "Since Rack is licensed under GPLv3+, you may license your plugin under GPLv3+ as well." **Your declared license needs no change.**
- There is **no narrow numeric "whitelist"** — the rule is **GPL-compatible / OSI open-source**. In-Library plugins use CC0, MIT, BSD-3-Clause, GPL-3.0, GPL-3.0-or-later, etc. GPL-3.0-or-later is the safest, most-recommended choice and is what you already have.
- **All assets you ship must be redistributable under the declared license** (or a compatible one). This is what makes finding **#7 (trial/proprietary fonts) a hard blocker**: publishing the repo is an act of redistribution. `BCBarellTEST-Regular.otf` (trial) and `FoundationLogo.ttf` must be removed from the working tree **and purged from git history** before the repo goes public. `res/fonts/JetBrainsMonoNL-Regular.ttf` is **OFL — safe to ship**.
- **Ethics guideline (enforced for Library):** "You may not clone the brand name, model name, logo, panel design, or layout of components … of an existing hardware or software product without permission." Forge Noir is an original design — no concern. Just ensure no character preset is named after a trademarked synth (PROJECT.md already lists "Named synth presets" as out of scope — good).
- **LICENSE file is required** (finding #5). GPL itself requires the full license text accompany the source; the Library expects it; your Makefile already lists `DISTRIBUTABLES += $(wildcard LICENSE*)` but no file exists. Add the verbatim GPLv3 text as `LICENSE` at repo root.

**Confidence: HIGH.** GPL-3.0-or-later acceptance is stated directly in the official manual.

---

## Part 3 — Submission Mechanism (the actual steps)

Verified against the VCVRack/library README.

### First-time submission (open-source)

1. **Publish the source repo** publicly on GitHub (milestone deliverable), after history purge of trial fonts (#7).
2. **Populate `plugin.json`** URLs: `sourceUrl` = `https://github.com/<user>/<repo>`, plus `authorUrl`, `pluginUrl`, `manualUrl`. Commit.
3. **Verify it builds clean** against the current Rack SDK and that `make dist` succeeds (Part 4) — VCV builds from your source; if it doesn't compile on their toolchain, it's rejected.
4. **Open exactly ONE issue** at https://github.com/VCVRack/library/issues with the **title equal to your plugin slug** (`ForgeAudio-AnalogSeries`) — *the slug, not the name*.
5. In the issue body, post: **plugin name, license, all relevant URLs (source/plugin/author/manual), and (optionally) a public support email.**
6. A Library maintainer picks it up, builds it for Mac/Windows/Linux from your source, and adds it. This issue thread is your **permanent channel** for all future updates to this plugin.

### Subsequent version updates (every release after the first)

1. **Increment `version` in `plugin.json`** (e.g. `2.0.0` → `2.0.1`). Required *before* you notify.
2. Push the commit to the public repo.
3. **Comment in the same issue** with the **new version number** AND the **exact commit hash** (`git rev-parse HEAD`). **Do not give a branch name like `main`/`master`** — the README explicitly forbids this; they pin the build to a commit.

### What you do NOT do

- **No pull request** against `VCVRack/library`. (The `manifests/*.json` PR flow is legacy.)
- **No uploading of the `.vcvplugin`** to VCV — they build from source. (You still build it locally for your own QA and for users who sideload before it lands in the Library; see Part 4.)
- No git **tags or GitHub Releases are required** by VCV — they pin to a commit hash, not a tag. (Tags are good practice for your own bookkeeping but are not part of the submission contract.)

> **Closed-source path (not yours):** email contact@vcvrack.com. Listed for completeness only.

**Confidence: HIGH** for the Issue+slug-title+commit-hash mechanism (README verbatim).

---

## Part 4 — Building Distributable Artifacts Locally

Verified against https://vcvrack.com/manual/Building.

Your Makefile is already correct and minimal:

```makefile
RACK_DIR ?= ../Rack-SDK
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)
include $(RACK_DIR)/plugin.mk
```

### Commands

```bash
export RACK_DIR=../Rack-SDK   # already your default via ?=

make            # compile plugin.so/.dylib/.dll
make dist       # build + package the distributable
make install    # build + package + copy into Rack user plugins dir (local test)
# make dep      # only if you had external deps — you don't, skip
```

### What `make dist` produces

- Output path: `dist/<slug>-<version>-<os>-<cpu>.vcvplugin`
  e.g. `dist/ForgeAudio-AnalogSeries-2.0.0-mac-arm64.vcvplugin`
- The `.vcvplugin` is a **compressed archive** (zip) containing: the compiled binary, **`plugin.json`**, and everything in `DISTRIBUTABLES` (**`res/`** and **`LICENSE`**).

### DISTRIBUTABLES checklist for your release

| Item | In Makefile? | Present on disk? | Action |
|------|--------------|------------------|--------|
| `res/` (panel SVG + OFL font) | yes | yes | OK |
| `LICENSE` | yes (`$(wildcard LICENSE*)`) | **NO** | **Create `LICENSE` (GPLv3 text)** — #5 |
| `presets/` | yes (`$(wildcard presets)`) | none | Fine — wildcard no-ops if absent |
| `plugin.json` | auto-included by `plugin.mk` | yes | OK |

> **Verification gate for the milestone's `.vcvplugin` deliverable:** after `make dist`, unzip the artifact and confirm `LICENSE`, `plugin.json` (with populated URLs), and `res/` are all present, and that **no trial fonts** are inside. The trial OTF/TTF live at repo root and are *not* in `res/`, so they won't be packaged — but they WILL be in the public git history until purged (#7).

**Confidence: HIGH.**

---

## Part 5 — Reviewer Criteria (what gets a plugin bounced)

Synthesized from the manual + ethics guidelines (MEDIUM confidence — VCV review is human and not exhaustively documented, but these are the consistently cited bars):

| Criterion | Bar | Your status |
|-----------|-----|-------------|
| **Compiles from source** on VCV's toolchain (all 3 OSes) | Hard requirement | Likely OK (C++17, no deps); verify clean build |
| **Open-source / compatible license** | Hard requirement | OK (GPL-3.0-or-later) + LICENSE file (#5) |
| **All shipped assets redistributable** | Hard requirement | **Blocked until #7 font purge** |
| **No IP/trademark cloning** (brand, panel, layout, names) | Hard requirement (ethics) | OK — original Forge Noir |
| **Not malware / no privacy harm** | Hard requirement | OK |
| **Valid manifest** (slug charset, valid tags, version format) | Hard requirement | OK once URLs added |
| **Panel quality / readability** | Soft — flagrant unreadability gets feedback | OK — polished Forge Noir |
| **CPU sanity** | Soft — no hard benchmark, but egregious waste noted | Watch finding #12 (display buffer on audio thread) at high instance counts; not a submission blocker |
| **Doesn't crash on patch load** | Soft but reputationally critical | **Fix #4** (malformed-JSON crash) before publishing |

There is **no published CPU benchmark threshold** and **no formal panel-art rubric** — review is pragmatic. The release blockers that actually gate *you* are #5 (LICENSE), #6 (URLs), and #7 (font purge). The crash guard (#4) is a strongly-advised pre-publish fix for reputation, not a manifest gate.

---

## Part 6 — C++ Test Framework: **doctest** (chosen)

**Recommendation: doctest, vendored as the single amalgamated header `tests/doctest/doctest.h`, pinned to v2.4.11 (stable line; v2.5.2 is the newest tag, Apr 2025).** Justification follows; Catch2 is the runner-up and is explicitly compared.

### Why doctest over Catch2

| Criterion | doctest | Catch2 v3 | Winner |
|-----------|---------|-----------|--------|
| Single-header drop-in | Yes — one `doctest.h`, zero build setup | v3 **dropped single-header**; now needs CMake-built static lib (v2 was header-only but is EOL) | **doctest** |
| Compile-time overhead of including the header | ~10ms per TU | ~430ms per TU | **doctest** (orders of magnitude) |
| Fits a `plugin.mk` Makefile with a hand-rolled test target | Trivial — add the header to include path | Awkward — wants CMake / a prebuilt lib | **doctest** |
| Added **runtime** dependency to the shipped plugin | **None** — tests compile in a separate target, never linked into `plugin.so` | None (also separate) | tie |
| C++17 support (your standard) | Yes (C++11/14/17/20/23) | Yes | tie |
| API ergonomics | Catch-like (`TEST_CASE`, `CHECK`, `REQUIRE`, `SUBCASE`) | `TEST_CASE`, `CHECK`, `REQUIRE`, `SECTION` | tie |

The decisive factors for *this* project: (1) **no build-system friction** — doctest is a header you check in, which matches a dependency-free `plugin.mk` setup; Catch2 v3 now mandates CMake, fighting your Makefile. (2) **Negligible compile cost**, so a test target stays fast. (3) **Zero runtime footprint** in the shipped plugin — the test executable is a wholly separate artifact and `doctest.h` never enters `src/*.cpp` that compiles into the plugin.

> Note on "single-header": doctest's *internal source tree* was modularized in 2.5.0, but the **distributed/amalgamated `doctest/doctest.h` remains the supported single-header consumption path** — you vendor that one file. Pin v2.4.11 for a battle-tested release, or v2.5.2 for newest; either is fine.

### Makefile integration sketch (separate target, no plugin coupling)

Keep the plugin Makefile pristine; add a self-contained test target that does **not** go through `plugin.mk` (so test code never links into the plugin). The clean pattern is to extract pure DSP into a header-or-`.cpp` with **no Rack dependency**, then compile only that against the test main.

```makefile
# --- appended to Makefile, after `include $(RACK_DIR)/plugin.mk` ---
# Pure DSP units under test must NOT include rack.hpp.
TEST_SRC   := tests/test_main.cpp $(wildcard tests/test_*.cpp)
TEST_DSP   := $(wildcard src/dsp/*.cpp)   # Rack-free DSP extracted here
TEST_BIN   := build/tests/run
TEST_FLAGS := -std=c++17 -Isrc -Itests/doctest -O0 -g

test:
	@mkdir -p build/tests
	$(CXX) $(TEST_FLAGS) $(TEST_SRC) $(TEST_DSP) -o $(TEST_BIN)
	./$(TEST_BIN)

.PHONY: test
```

```cpp
// tests/test_main.cpp — the only TU that defines the runner
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
```

```cpp
// tests/test_clock.cpp — example regression test (finding #1)
#include "doctest/doctest.h"
#include "ClockTracker.hpp"   // extracted, Rack-free
TEST_CASE("clock re-locks after >3x tempo jump") {
    ClockTracker t;
    lockTo(t, /*period=*/0.5);          // 120 BPM
    feedEdges(t, /*period=*/0.125, 4);  // 4x faster
    CHECK(t.state == ClockTracker::LOCKED);
    CHECK(t.smoothedPeriod == doctest::Approx(0.125).epsilon(0.05));
}
```

**Headless integration harness** (the milestone's `process()`-driver requirement): the same `test` binary can host it, but driving `Module::process()` pulls in `rack.hpp`. Two clean options:
- **(a) Preferred** — Extract the engine into a Rack-free `Engine` struct (taking a plain `{float sampleTime}` arg), drive it in doctest, assert on output blocks. Keeps everything in one fast, dependency-free `make test`.
- **(b)** If full `Module` coverage is wanted, build a second target that links the Rack SDK; heavier, only if (a) leaves real gaps.

Each fixed bug (#1–#4) lands as a doctest `TEST_CASE`, satisfying the milestone's "bug fixes as regression tests" goal.

---

## What NOT to bother adding

| Skip | Why |
|------|-----|
| GitHub Release / git tag *for the submission* | VCV pins to a commit hash, not a tag. (Tags optional for your own records.) |
| Uploading `.vcvplugin` to VCV | They build from source; the artifact is for your QA + early sideloaders only |
| A PR to `VCVRack/library` | Submission is an Issue titled with your slug |
| `manifests/*.json` file | Legacy flow; not part of current Issue-based submission |
| `donateUrl`, `modularGridUrl`, module `hidden`, `keywords` (optional) | Not needed for a clean first ship; add later if wanted |
| `Polyphonic` tag | Module is monophonic by design — would be a false tag |
| Catch2 / GoogleTest / Boost.Test | Build-system friction vs `plugin.mk`; doctest wins on header-drop + compile speed |
| `minRackVersion: 2.6.0` *as-is* (reconsider) | Excludes 2.0–2.5 users; lower unless a 2.6 API is actually used |

---

## Concrete v1.4 release checklist (derived)

1. Purge `BCBarellTEST-Regular.otf` + `FoundationLogo.ttf` from working tree **and git history** (#7) — do this *before* the repo is ever pushed public.
2. Add GPLv3 `LICENSE` at repo root (#5).
3. Fill `authorUrl`, `pluginUrl`, `sourceUrl` (GitHub project page), add `manualUrl` (Notion) in `plugin.json` (#6). Reconsider `minRackVersion`.
4. Confirm slug `ForgeAudio-AnalogSeries` + module slug `ForgeAnalogLFO` are final (permanent).
5. Fix patch-load crash (#4) — reputation gate.
6. `make dist`; unzip; verify `LICENSE` + populated `plugin.json` + `res/` present, no trial fonts inside.
7. Push public GitHub repo.
8. Open one Library issue titled `ForgeAudio-AnalogSeries`; post name, license, source/plugin/author/manual URLs, commit hash.
9. For later updates: bump `plugin.json` version → push → comment new version + `git rev-parse HEAD` in the same issue.

---

## Sources

- https://vcvrack.com/manual/Manifest — manifest field list, required/optional, slug charset, version format, tag vocabulary (HIGH)
- https://vcvrack.com/manual/PluginLicensing — GPL-3.0-or-later accepted/recommended, ethics/trademark rules, asset licensing (HIGH)
- https://vcvrack.com/manual/Building — `make`/`make dist`/`make install`, `.vcvplugin` path & contents, DISTRIBUTABLES (HIGH)
- https://github.com/VCVRack/library (+ README) — Issue-based submission, slug-titled issue, source URL + version + commit-hash update flow, no-branch-name rule (HIGH)
- https://github.com/doctest/doctest + releases — single-header `doctest.h`, v2.5.2 (Apr 2025) / v2.4.11 stable, C++11–23 (HIGH)
- https://accu.org/journals/overload/25/137/kirilov_2343/ and https://hackingcpp.com/cpp/tools/testing_frameworks — doctest ~10ms vs Catch ~430ms include overhead; Catch2 v3 dropped single-header (MEDIUM/HIGH)
- VCV Community (community.vcvrack.com) — GPL relicensing context, in-Library licenses include CC0/MIT/BSD/GPL (MEDIUM)

---
*Stack research for: VCV Library release & packaging (v1.4 Tempered)*
*Researched: 2026-06-14*
