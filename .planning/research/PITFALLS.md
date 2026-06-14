# Pitfalls Research

**Domain:** Releasing / hardening a VCV Rack 2 plugin (private working repo → public GPL source → VCV Library submission)
**Researched:** 2026-06-14
**Confidence:** HIGH (git-filter-repo, VCV Manifest, VCV submission rules verified against official sources; repo state verified directly via git/gh)

---

## Repo-state facts that change the standard advice (verified this session)

These were checked directly and override generic guidance:

- **`origin` already exists and history is already pushed.** Remote: `https://github.com/Photep/ForgeAudio-AnalogSeries.git`. `gh repo view` reports `"visibility":"PRIVATE"`, `pushedAt 2026-06-13`, `forkCount 0`, `isFork false`. The trial fonts are **already on GitHub's servers** inside commit `e486ce1` ("design: add Forge Noir panel mockup and design language"). So the framing is *not* "purge before first push" — it is **"purge from local history AND force-push the rewritten history to the existing private remote, BEFORE the repo is ever flipped to public."**
- **Both trial fonts entered in exactly one commit:** `e486ce1`. `BCBarellTEST-Regular.otf` and `FoundationLogo.ttf` are tracked at repo root; only that one commit ever touched them. This makes the purge surgically simple (one filter pass, one commit affected) but the file must still be scrubbed from *every* commit reachable after `e486ce1`.
- **`res/fonts/JetBrainsMonoNL-Regular.ttf` is the ONLY font referenced by source** (`src/AnalogLFO.cpp:1354`). It is JetBrains Mono = OFL (SIL Open Font License) = redistributable. The trial fonts are mockup-only and unreferenced by code — safe to delete outright, not just untrack.
- **No `LICENSE` file, no `.gitignore` font rule, no `tests/` dir, no `CHANGELOG`** currently exist.
- **`plugin.json` version is `2.0.0`, not `1.4.x`.** This is *correct* for VCV (MAJOR must equal the target Rack major version = 2) but collides with the internal milestone label "v1.4" — see Pitfall 8.
- **Manifest URLs (`authorUrl`/`pluginUrl`/`sourceUrl`) are all empty;** tags present (`Low-frequency oscillator`, `Waveshaper`), slug `ForgeAudio-AnalogSeries`, module slug `ForgeAnalogLFO`.

---

## Critical Pitfalls

### Pitfall 1: Removing the trial fonts only from HEAD, leaving them in history

**What goes wrong:**
You `git rm BCBarellTEST-Regular.otf FoundationLogo.ttf`, commit, and call it done. The files are gone from the working tree and from the tip commit, but they remain fully downloadable from commit `e486ce1` (and every commit between it and the deletion). The moment the GitHub repo is flipped to **public**, anyone can `git checkout e486ce1` or browse the tree at that SHA and pull the trial/proprietary font binaries — which is exactly the redistribution the trial license prohibits and which violates VCV's "do not misuse intellectual property (legally or morally)" acceptance rule.

**Why it happens:**
Git's mental model is "the latest commit is the state of the repo." Deleting from HEAD *feels* complete. Developers forget that public repos expose the **entire reachable object graph**, not just `HEAD`.

**How to avoid — exact, ordered procedure (repo is private+pushed, so this is the safe path):**

1. **Confirm the repo is still PRIVATE before doing anything destructive.** It is today; do not flip to public until step 9.
   `gh repo view Photep/ForgeAudio-AnalogSeries --json visibility`
2. **Finish/commit/stash all real work first.** History rewrite changes every commit SHA from `e486ce1` onward; do it on a quiet tree with nothing else in flight, ideally as its own milestone phase.
3. **Operate on a FRESH clone, not the working repo.** `git filter-repo` refuses to run on a repo that has a remote unless `--force`, specifically to protect you. Honour that:
   ```bash
   cd /tmp
   git clone --no-local "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" forge-clean
   cd forge-clean
   ```
4. **Verify git-filter-repo is installed** (`pip install git-filter-repo` or `brew install git-filter-repo`): `git filter-repo --version`.
5. **Purge the two files from ALL history:**
   ```bash
   git filter-repo \
     --path BCBarellTEST-Regular.otf \
     --path FoundationLogo.ttf \
     --invert-paths
   ```
   `--invert-paths` keeps everything *except* the listed paths, deleting them from every commit. (Equivalent: `--path-glob '*.otf'` only if you are certain no shippable .otf exists — here the OFL font is `.ttf`, so the explicit two-path form is preferred for auditability.)
6. **Re-add the remote** (filter-repo deletes `origin` by design, to stop you accidentally mixing old + rewritten history):
   ```bash
   git remote add origin https://github.com/Photep/ForgeAudio-AnalogSeries.git
   ```
7. **Force-push the rewritten history to the still-private remote** (overwrites the contaminated server-side history):
   ```bash
   git push --force --all origin
   git push --force --tags origin
   ```
8. **Re-point the real working repo at the rewritten history.** Because SHAs changed, the original repo is now divergent. Simplest safe move: re-clone the rewritten remote into a fresh working dir and continue there, OR in the existing repo `git fetch origin && git reset --hard origin/main` (only after confirming no un-pushed local work exists).
9. **Only now flip to public:** `gh repo edit Photep/ForgeAudio-AnalogSeries --visibility public --accept-visibility-change-consequences`.

**Verifying the purge (do all four):**
```bash
# 1. No commit touches that path anywhere in history:
git log --all --oneline -- BCBarellTEST-Regular.otf FoundationLogo.ttf   # → empty
# 2. Not in any tree at any commit:
git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'          # → empty
# 3. The (now re-hashed) introducing commit no longer carries them:
git show <new-sha-of-e486ce1> --stat | grep -iE 'otf|FoundationLogo'     # → empty
# 4. After force-push, confirm server-side via a throwaway fresh clone:
git clone https://github.com/Photep/ForgeAudio-AnalogSeries.git /tmp/verify && \
  cd /tmp/verify && git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'   # → empty
```
Note: GitHub may keep unreachable objects cached and accessible by exact SHA via the API until GC. **Doing the purge while still PRIVATE sidesteps this entirely** because no third party ever had access — the single biggest reason to purge-before-public rather than purge-after.

**BFG alternative:** `bfg --delete-files '{BCBarellTEST-Regular.otf,FoundationLogo.ttf}'` then `git reflog expire --expire=now --all && git gc --prune=now --aggressive`. BFG is faster on huge repos, but git-filter-repo is the officially recommended tool (git's own docs deprecate `filter-branch` in its favour), is already path-precise here, and auto-removes the remote as a guardrail. **Recommend git-filter-repo.**

**Warning signs:** "I deleted the fonts" with no mention of a history rewrite; a PR that only modifies HEAD; the repo being made public before a verification clone was checked.

**Phase to address:** Dedicated **"Release IP hardening / history purge"** phase, executed while repo is still private, BEFORE the "publish public repo" phase. Must be its own phase because it rewrites every SHA and gates publication.

---

### Pitfall 2: Going public (or submitting) before the purge is *verified*, not just performed

**What goes wrong:**
The publish step and the purge step get bundled into one phase, the force-push silently pushes only `main` (not `--all`/`--tags`), and the repo is flipped public with contaminated history still on a stale branch/tag.

**Why it happens:**
`git push --force origin main` only rewrites one ref. Other branches/tags created before the rewrite still point at old commits that contain the fonts.

**How to avoid:**
- Force-push with `--all` and `--tags`, not a single branch.
- Treat publication as a **separate phase gated on the four-step verification above passing against a fresh clone of the remote.**
- Make "fresh-clone grep returns empty" a hard release-blocker checklist item.

**Warning signs:** Extra branches/tags on the remote; verification done against the local clone only, never against a re-clone of the remote.

**Phase to address:** "Publish public repo" phase — entry criterion = purge verification passed.

---

### Pitfall 3: Shipping or crediting incompatible third-party assets (license/IP traps)

**What goes wrong:**
GPL-3.0-or-later requires everything distributed *as part of the work* to be GPL-compatible, properly licensed, and attributed. Risks for this repo:
- **Trial fonts** (`BCBarellTEST-Regular.otf` "TEST" suffix, `FoundationLogo.ttf`): trial/eval licenses almost universally forbid redistribution → cannot live in a public repo at all (covered by Pitfalls 1–2).
- **JetBrains Mono (OFL):** redistributable and GPL-compatible, BUT OFL requires the license text be included and the font not be sold by itself. Shipping it inside a GPL plugin is fine; still include the OFL license text / attribution.
- **Panel SVG / knob art:** `res/AnalogLFO.svg` and `res/components/*` must be your own work or licensed for redistribution under terms compatible with GPL-3.0. Any traced logo, borrowed knob graphic, or export of a copyrighted glyph is an IP risk. If panel text was converted to outlines from a non-OFL font, those *paths* may be a derivative requiring a redistribution license.
- **Bebas Neue / Chakra Petch:** named as brand fonts in PROJECT.md. Confirm they were used only to render *mockups* (PNGs, not committed source), OR — if their outlines were baked into the production SVG — that the *specific files used were the OFL versions* (both are OFL on Google Fonts), not a trial/commercial cut.

**Why it happens:**
"It builds and looks right" hides licensing status. The same typeface ships under wildly different licenses (free OFL vs paid foundry cut vs trial). The "TEST" suffix here is the giveaway.

**How to avoid:**
- Inventory every binary/art asset in `res/` and at repo root; record each asset's license in a `LICENSE` + `NOTICES` (or `res/fonts/README`) file.
- Keep only OFL/CC-BY/own-work assets; purge trial/commercial ones from history.
- Add the OFL license text for JetBrains Mono (and any other OFL font whose outlines ship in the SVG) alongside the GPL `LICENSE`.
- Confirm via `grep` that source references only OFL assets (already verified: only `JetBrainsMonoNL-Regular.ttf` is referenced).

**Warning signs:** Font filenames with "TEST"/"Trial"/"Demo"/foundry names; SVG with embedded base64 fonts; logo art with no provenance note.

**Phase to address:** "Release IP hardening" phase (same one that does the purge) — produces the asset inventory + NOTICES file. LICENSE file creation (finding #5) also lands here.

---

### Pitfall 4: Trademark / brand-naming traps in the VCV Library

**What goes wrong:**
VCV's acceptance rule is explicit: plugins that "misuse intellectual property (legally or morally)" are rejected. Naming or marketing a module after trademarked hardware ("Minimoog", "Moog", "Roland", "Juno", "Prophet", "SH-101", "MPC") in user-facing strings (module name, keywords, description, panel text, preset names) invites takedown. PROJECT.md's character knob *targets* those synths internally — fine as engineering intent — but the **shipped** name/description/panel must not claim or imply endorsement or use the trademarks as product identifiers.

**Why it happens:**
The character modeling genuinely references those synths, so it feels natural to name them in the UI/manual for clarity.

**How to avoid:**
- Keep "Minimoog/Roland/Moog/Prophet" out of `plugin.json` `name`/`description`/`keywords`, panel text, and preset names. Generic descriptors ("classic ladder character", "vintage transistor saw") are safe.
- "Forge Audio" / "Analog LFO" / "Forge Noir" appear original — do a quick marketplace + USPTO/EUIPO name search to confirm no collision with an existing VCV plugin slug or brand (search the VCV Library + the library GitHub issue tracker for "Forge").
- The Notion manual *may* describe reference synths in prose ("inspired by vintage transistor LFO character") but must avoid trademark-as-feature-name.

**Warning signs:** Trademarked names in any user-facing string; "official"/"authentic Moog" style claims.

**Phase to address:** "VCV Library compliance" phase (manifest/strings audit) + "Manual" phase (prose review).

---

### Pitfall 5: VCV Library manifest / submission rejection causes

**What goes wrong (concrete, mapped to this repo):**

| Cause | This repo's status | Fix |
|---|---|---|
| Missing `sourceUrl` (no way to fetch source) | **EMPTY** — blocker | Fill after public repo exists |
| Empty `authorUrl`/`pluginUrl`/`manualUrl` | empty (optional, but fill `pluginUrl`/`manualUrl`→Notion) | populate |
| `version` MAJOR ≠ Rack major | `2.0.0` is **correct** (Rack 2). Do NOT change to `1.4.x` | leave at 2.x; bump REVISION |
| Slug changed after release | slug `ForgeAudio-AnalogSeries`, module `ForgeAnalogLFO` — **must never change** once submitted | freeze slugs now |
| Slug illegal chars | only `a-zA-Z0-9-_` allowed; current slugs legal | ok |
| Missing/invalid tags | tags must match VCV's fixed list, case-insensitive; `Low-frequency oscillator` + `Waveshaper` both valid | ok |
| Panel SVG path mismatch | panel is `res/AnalogLFO.svg`, module slug is `ForgeAnalogLFO`; Rack loads whatever path the code passes to `createPanel` — verify the load path matches and the SVG renders in the official build | verify load path |
| Unsupported SVG (filters/CSS/blur/live text) | PROJECT.md constrains to nanosvg subset; re-verify no `<filter>`, gradients-on-text, or live `<text>` survive in production SVG | lint SVG |
| Build fails on official toolchain | must build clean with the Rack SDK on Mac/Win/Linux | CI build all three |
| Commit hash not provided / branch name given | submission needs exact `git rev-parse HEAD`, not "main" | record SHA at submit |
| Makefile `VERSION` not incremented | VCV reads Makefile `VERSION` for updates | keep Makefile VERSION == plugin.json |
| Closed-source / proprietary license declared but no LICENSE | declared `GPL-3.0-or-later` but no `LICENSE` ships | add LICENSE (finding #5) |
| IP misuse (trial fonts, trademarks) | trial fonts in history (Pitfall 1), trademark strings (Pitfall 4) | purge + string audit |

**Why it happens:**
The manifest "looks complete" but several fields are empty or only matter at submission time; SVG renders in the dev build but not under the official renderer.

**How to avoid:**
- Validate `plugin.json` against the Manifest spec field-by-field; fill all submission-required URLs.
- Build with the official Rack SDK and load the panel in a real Rack build (not just the dev shortcut) to catch SVG render issues.
- One issue thread titled with the **slug** (`ForgeAudio-AnalogSeries`); post sourceUrl + commit hash + license + version.
- Keep Makefile `VERSION` == `plugin.json` `version`.

**Warning signs:** Empty URL fields; panel that looks different in dev preview vs a fresh Rack install; SVG containing `<filter>`/`feGaussianBlur`/live text.

**Phase to address:** "VCV Library compliance" phase (manifest + LICENSE + SVG lint + build verify) and a final "Submission" phase (issue thread + commit hash).

---

### Pitfall 6: Regression when refactoring DSP for testability (findings #11, #12)

**What goes wrong:**
- **#12 (move display-buffer generation off the audio thread):** `updateDisplayBuffer()` currently runs in `process()`. Moving its 256× `computeMorphedWave` to `WaveformDisplay::step()` (GUI thread) introduces a **thread boundary** the code didn't have. Risks: reading morph/character/swing without the existing atomic bridges (tearing / stale frame); display lagging audio by a variable number of frames; the buffer being read by the widget while half-written (needs the existing lock-free double-buffer discipline, not a naive shared array); behavior diverging clocked vs unclocked because the swing-zeroing gate (`isClocked ? swingFrac : 0.5f`, see finding #3) must move with it.
- **#11 (frame-rate-independent timing via `getLastFrameDuration`):** Replacing fixed `1/60` ticks with wall-clock deltas changes breathe/blink/scanline math. Risks: a huge `dt` on the first frame or after a stall (window unfocused, patch load) causing a visible jump/flash; SYNC blink rate drifting if `dt` is sampled inconsistently; tests that assumed 60 ticks/sec breaking.

**Why it happens:**
Refactoring "for testability" touches timing and threading — the two areas with the least deterministic, hardest-to-eyeball behavior. The code "has shipped fine through four milestones," so the safety margin masking #12 is invisible until instance count or sample rate changes.

**How to avoid — guard with the NEW tests this milestone adds:**
- **Extract pure functions first, test, then move.** Pull `computeMorphedWave` / buffer generation into a side-effect-free function unit-tested for identical output given the same morph/character/swing — *before* relocating the call site. The test pins behavior so the move is provably output-preserving.
- **Headless `process()`-driver harness:** run N sample blocks at multiple sample rates (44.1k/48k/96k) and multiple block sizes; assert output is bit-identical (or within epsilon) before vs after the #12 move. Catches block-buffering assumptions.
- For #11: clamp `dt` (e.g. `dt = clamp(getLastFrameDuration(), 0, 1/30.0)`) so a stalled frame can't fling an animation; unit-test a pathological large `dt` and assert phase advances only by the clamped amount.
- Keep the atomic bridges for any value crossing the new thread boundary; reuse the existing lock-free double-buffer pattern, not a raw shared buffer.
- **Land each bug fix (#1–#4) as a regression test in the same commit** (PROJECT.md mandates this) so the refactor can't silently reintroduce them.

**Warning signs:** Display "looks slightly off" after refactor; crackle reports as instance count grows (the #12 margin finally exhausted); animation jumps on patch load / window refocus; tests that hard-code 60fps.

**Phase to address:** "Test harness" phase (build unit + headless harness FIRST), then "DSP/display refactor" phase gated on those tests being green before and after each change. #12 and #11 must not precede the harness.

---

### Pitfall 7: Committing a behavioral change (x1.5 / ÷1.5 retrigger, finding #2) without auditioning

**What goes wrong:**
Finding #2's proposed per-ratio alignment table is *mathematically* more consistent, but the current every-beat retrigger on x1.5 might be a **desirable groove** the author/users actually like. "Correct by math" can be "worse by feel." Committing the table change blind risks shipping a regression in musical behavior that no automated test catches (it's a taste/feel decision, not a correctness bug).

**Why it happens:**
DSP correctness instincts treat "inconsistent with other ratios" as a bug to fix. For a creative instrument, intentional asymmetry can be a feature.

**How to avoid — explicit audition gate:**
1. Build both versions behind a temporary toggle (or two builds: current vs aligned table).
2. Audition x1.5 and ÷1.5 in Rack against a clock with a musical patch: listen for whether the every-beat chop is a groove or an artifact; watch the phase dot teleport vs glide.
3. Record the decision (with audio if possible) in the milestone log: keep-current vs adopt-table, with rationale.
4. Only after the listening decision: implement the chosen behavior and add a **deterministic regression test** pinning the *chosen* alignment (e.g. "÷1.5 resets every 3 beats", or "x1.5 retriggers every beat — intentional") so future refactors can't silently flip it.
5. If the table is adopted, also assert no mid-cycle truncation (phase ≈ 0 at the reset boundary) in the headless harness.

**Warning signs:** A PR that changes the ratio table with no "auditioned" note; a test that encodes the *math* without anyone having listened; user feedback that "the groove changed."

**Phase to address:** "Bug fixes" phase, with #2 explicitly **audition-gated** (PROJECT.md already flags it). The listening decision is a phase entry criterion for writing the test.

---

### Pitfall 8: Version / changelog hygiene for a public release

**What goes wrong:**
- **Internal milestone "v1.4" vs manifest `version 2.0.0` confusion.** A well-meaning fix could "correct" the manifest to `1.4.0`, which would **break VCV** (MAJOR must equal Rack major = 2) and confuse the Library updater. The internal milestone name and the published plugin version are *different numbering schemes* and must not be conflated.
- **Makefile `VERSION` drifting from `plugin.json version`** — VCV reads the Makefile for update detection; mismatch makes the Library miss/misreport the release.
- **No `CHANGELOG`** (confirmed absent). A public release with no changelog leaves users and the reviewer without a record of what changed; `changelogUrl` goes unfilled.
- **First public version chosen carelessly** — since the slug is frozen forever, the first published `version` is the baseline; pick a clean `2.x.y` and only ever increment.

**Why it happens:**
Two parallel version systems (GSD milestone vs VCV manifest) invite "fixing" the one that looks wrong.

**How to avoid:**
- Document the rule: **manifest stays 2.x (Rack 2); milestone label v1.4 is internal only.** Increment REVISION/MINOR for this release (e.g. keep `2.0.0` for the first public build, or `2.1.0` if treating the hardening as a feature release).
- Keep `plugin.json version` and Makefile `VERSION` identical; add a grep-compare check to the release checklist.
- Add `CHANGELOG.md`; set `changelogUrl` and `manualUrl` (Notion) in the manifest.
- Tag the release commit (`git tag v2.x.y`) and record `git rev-parse HEAD` for the VCV submission.

**Warning signs:** Anyone editing `version` toward `1.x`; Makefile and plugin.json versions differing; release with no tag/changelog.

**Phase to address:** "VCV Library compliance" phase (version reconciliation + CHANGELOG + Makefile sync) and "Submission" phase (tag + commit hash).

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Delete trial fonts from HEAD only, skip history rewrite | One commit, done in seconds | Fonts redistributable forever once public; license violation + VCV rejection grounds | **Never** for a repo going public |
| Flip repo public, then purge | Unblocks submission sooner | Third parties (and GitHub SHA-addressable cache) may already have the fonts; purge no longer fully effective | Never — purge while private |
| Move display buffer off audio thread without extracting/pinning the pure function first | Faster refactor | No proof output is preserved; silent visual/audio regression | Never this milestone (tests are the whole point) |
| Adopt the #2 alignment table on math grounds without auditioning | Closes the finding fast | May ship a worse groove; un-catchable by tests | Never — audition first |
| Set manifest version to "1.4.0" to match milestone | Naming feels consistent | Breaks VCV MAJOR=Rack-major rule; Library update confusion | Never |
| Skip CHANGELOG for first public release | Less writing | No record for users/reviewer; empty changelogUrl | Only for a truly internal pre-release, not the public submission |
| Hard-code 60fps to keep #11 simple | Less math | Animations wrong on 120/144Hz displays (the bug being fixed) | Never — defeats finding #11 |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| `git filter-repo` | Running it on the live working repo; surprised origin vanished; force-pushing only `main` | Run on a fresh `--no-local` clone; re-add origin; `push --force --all --tags`; re-sync working repo |
| GitHub visibility | Making repo public before purge verified against a fresh remote clone | Verify (4-step grep on a re-clone) while still PRIVATE, then flip public |
| VCV Library issue tracker | Titling the thread with the plugin *name*; giving a branch name | Title = **slug** (`ForgeAudio-AnalogSeries`); post `git rev-parse HEAD` + sourceUrl + license |
| VCV manifest ↔ Makefile | `plugin.json version` and Makefile `VERSION` drift | Keep identical; grep-compare in release checklist |
| Panel SVG ↔ nanosvg renderer | Filters/blur/CSS/live `<text>` that render in editor but not in Rack | Lint SVG to nanosvg subset; load in a real Rack build |
| Notion manual ↔ manifest | Manual exists but `manualUrl`/`pluginUrl` left empty | Publish Notion page, paste its public URL into the manifest before submission |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Display buffer regen on audio thread (#12) | Crackle/xruns as instance count rises | Move to GUI `step()` behind atomics + double-buffer | Many instances / high sample rate / low block size |
| Frame-rate-coupled animation (#11) | Breathing glow ~2.4× too fast on 144Hz; SYNC blink wrong | Wall-clock `dt` (clamped) | Any monitor >60Hz |
| Unclamped `dt` after stall | Animation jump/flash on patch load or window refocus | `clamp(dt, 0, 1/30)` | First frame, focus changes, heavy patches |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Patch-load crash on malformed JSON (#4) | Hand-edited/corrupt patch crashes Rack (DoS-ish, user data loss) | `json_is_string()` guard + try/catch or `strtoull`; fall back to existing seed |
| Trial/proprietary binaries in public history | Legal exposure (license violation), VCV rejection | History purge before public (Pitfall 1) |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| "Fixing" x1.5 groove without auditioning (#2) | Familiar groove silently changes; users feel the instrument regressed | Audition gate, document decision, pin with test |
| Animations at wrong speed on high-Hz displays (#11) | Visual identity (breathing ember glow, SYNC flash) feels wrong | Frame-rate-independent timing |
| Phase dot desync with swing in free-run (#3) | Dot no longer tracks actual output voltage — misleading feedback | Store effective display value (`isClocked ? swingFrac : 0.5f`) |

## "Looks Done But Isn't" Checklist

- [ ] **Font removal:** HEAD is clean — verify `git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'` is empty on a fresh clone of the *remote*.
- [ ] **LICENSE:** manifest says GPL-3.0-or-later — verify an actual `LICENSE` file exists at repo root AND Makefile `DISTRIBUTABLES` picks it up.
- [ ] **Manifest URLs:** look populated — verify `sourceUrl` reachable, `pluginUrl`/`manualUrl` point to live public pages.
- [ ] **Version:** plugin.json `2.x` — verify it was NOT "corrected" to 1.4.x and matches Makefile `VERSION`.
- [ ] **Panel SVG:** renders in dev — verify it renders identically in a clean official Rack build, no unsupported SVG features.
- [ ] **Refactor (#11/#12):** "works on my machine" — verify headless harness output is identical pre/post at 44.1/48/96k, and animation tested on a >60Hz display (or simulated large `dt`).
- [ ] **#2 ratio change:** test is green — verify a human actually auditioned and the decision is logged.
- [ ] **Bug fixes #1–#4:** marked fixed — verify each has a regression test that fails on the old code.
- [ ] **CHANGELOG:** release tagged — verify CHANGELOG.md updated and a `v2.x.y` git tag exists.
- [ ] **Submission:** thread opened — verify title = slug, commit hash (not branch) posted.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Fonts leaked to a public repo before purge | HIGH | Purge history + force-push; assume binaries are "out there"; if a trial license was breached, stop distribution and document; GitHub support can be asked to GC cached SHAs but assume copies exist |
| Manifest version set to 1.x and submitted | MEDIUM | Bump back to 2.x, increment Makefile VERSION, post corrected commit hash in the VCV thread |
| #12 refactor introduced crackle/regression | MEDIUM | Revert to pre-refactor commit (kept green by tests), re-extract pure function, re-test before re-moving |
| #2 groove changed and users complain | LOW–MEDIUM | Toggle back to prior behavior (kept behind a flag during audition), re-audition, re-pin test |
| Slug changed after release | HIGH | Cannot undo cleanly — breaks every saved patch; must restore original slug |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| 1. Fonts only removed from HEAD | Release IP hardening / history purge (while PRIVATE, before publish) | 4-step grep empty on a fresh clone of the remote |
| 2. Public/submit before purge verified | Publish public repo (entry-gated on purge verification) | Fresh-clone grep empty; `--all --tags` force-pushed |
| 3. Incompatible/uncredited assets | Release IP hardening (asset inventory + LICENSE/NOTICES, finding #5) | NOTICES lists every asset's license; source refs only OFL |
| 4. Trademark naming | VCV compliance (string audit) + Manual phase | No trademarked names in manifest/panel/preset/manual UI strings |
| 5. Manifest/submission rejection | VCV Library compliance + Submission | Field-by-field manifest validation; official-build panel render; slug-titled thread + commit hash |
| 6. DSP refactor regression (#11/#12) | Test harness phase (FIRST) → DSP/display refactor phase | Headless harness bit-identical pre/post at 3 sample rates; clamped-`dt` test |
| 7. #2 committed without audition | Bug fixes phase (#2 audition-gated) | Logged listening decision; regression test pins chosen behavior |
| 8. Version/changelog hygiene | VCV compliance (version + CHANGELOG + Makefile sync) → Submission (tag + hash) | plugin.json==Makefile version, MAJOR=2, CHANGELOG present, release tagged |

**Recommended phase ordering (purge safety is load-bearing):**
1. Test harness (unit + headless) — enables safe refactors and bug-fix regression tests
2. Functional bug fixes #1, #3, #4 + #2 (audition-gated) — each lands with a test
3. Code/display cleanups + refactors #8–#12 — gated on harness green
4. Release IP hardening: LICENSE (#5), manifest URLs (#6), asset inventory/NOTICES, **history purge of trial fonts (#7) while PRIVATE**, verify purge
5. VCV compliance: manifest validation, version/CHANGELOG/Makefile reconciliation, SVG lint, official-build verify, string/trademark audit
6. Publish public repo (gated on purge verification) → Notion manual published → fill `manualUrl`/`pluginUrl`/`sourceUrl`
7. Submission: tag release, capture commit hash, open VCV issue thread titled with the slug

## Sources

- git-filter-repo — official repo & usage (`--path`, `--invert-paths`, fresh-clone/`--force` guardrail, removes origin, force-push requirement): https://github.com/newren/git-filter-repo
- git-filter-repo practical removal guides (corroborating exact flags, re-add remote, force-push, re-clone): https://www.git-tower.com/learn/git/faq/git-filter-repo , https://coreui.io/answers/how-to-remove-a-file-from-git-history/ , https://marcofranssen.nl/remove-files-from-git-history-using-git-filter-repo
- VCV Plugin Manifest — field requirements, slug rules ("never change" after release, `a-zA-Z0-9-_`), version (MAJOR=Rack major, no "v" prefix), SPDX license, valid module tag list, `res/<module slug>.svg`: https://vcvrack.com/manual/Manifest
- VCV Library submission rules — issue thread titled with slug, post sourceUrl + commit hash (not branch), increment Makefile VERSION, "do not misuse intellectual property (legally or morally)" acceptance rule: https://github.com/VCVRack/library/blob/master/README.md
- VCV Panel Guide — SVG saved to `res/<module slug>.svg`, nanosvg subset constraints: https://vcvrack.com/manual/Panel
- SIL Open Font License (OFL) — JetBrains Mono / Bebas Neue / Chakra Petch redistribution & attribution terms: https://openfontlicense.org/
- Repo state verified directly this session: `git ls-files`, `git log --all -- <fonts>` (both fonts only in commit `e486ce1`), `git ls-remote`, `gh repo view ... --json visibility,pushedAt,forkCount` (PRIVATE, already pushed), `plugin.json` inspection, `grep` of source (only `JetBrainsMonoNL-Regular.ttf` referenced), absence of `LICENSE`/`CHANGELOG`/`tests/`/font `.gitignore`.

---
*Pitfalls research for: VCV Rack 2 plugin release hardening (Forge Audio — Analog LFO, v1.4 Tempered)*
*Researched: 2026-06-14*
