# Phase 25: Release IP Hardening (PRIVATE) - Research

**Researched:** 2026-06-30
**Domain:** Git history rewriting (trial-font purge), open-source licensing (GPL-3.0 + OFL), asset provenance for public GPL release
**Confidence:** HIGH (repo facts verified directly; licensing verified against OFL-FAQ + Google Fonts)

## Summary

Phase 25 makes the repo legally clean for a public GPL-3.0 release while it is still PRIVATE. Three things must happen: (1) remove the two trial fonts from the working tree and gitignore them; (2) purge those font blobs from **all** git history and force-push to the private remote, then prove they are gone via a fresh re-clone; (3) add a GPL-3.0 `LICENSE` and a NOTICES/credits file, and confirm the panel-art font-outline provenance is acceptable for redistribution.

I verified the live repo state directly. Both trial fonts (`BCBarellTEST-Regular.otf`, `FoundationLogo.ttf`) sit at repo root, are **git-tracked**, were introduced in commit `e486ce1`, and persist in every tree from there to HEAD. They are **not referenced** by any build file, source, or shipped SVG — they are mockup-only. The shipped panel `res/AnalogLFO.svg` contains **120 `<path>` elements and ZERO `<text>`, `<tspan>`, `<glyph>`, `<font>`, `@font-face`, or embedded base64/font data** — all wordmark text is baked as vector outlines, so **no font program ships in the SVG**. The only runtime font that ships is `res/fonts/JetBrainsMonoNL-Regular.ttf` (OFL).

**One critical, easily-missed fact drives the whole purge sequence:** the local `main` is **75 commits AHEAD of `origin/main`**. The remote is parked at the v1.3 release (`2cf7b59`, == tag `v1.3`); none of the Phase 22-24 work is pushed. The naive "fresh-clone-of-remote → filter → force-push" recipe would silently **drop 75 commits**. The trial-font commit `e486ce1` is an ancestor of the remote, so the fonts exist on the remote too. The safe sequence must push local work to the remote *first* (after the `git rm`), then purge.

**Primary recommendation:** Three plans. (P1) `git rm` the two fonts + gitignore + add `LICENSE` (GPL-3.0) + add `NOTICES` + bundle `OFL.txt`, commit, confirm `make`/`make dist` still ship correctly. (P2 — human-gated) Confirm the FORGE/AUDIO wordmark outline provenance (Bebas Neue OFL vs. the trial FoundationLogo) — the single real IP risk. (P3 — irreversible, gated) push local→remote, fresh-clone, `git filter-repo --invert-paths`, force-push `--all`+tags, verify clean via a second fresh clone.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Trial-font removal from working tree | Repo / VCS | Build (gitignore) | `git rm` + `.gitignore`; no code touches these files |
| Trial-font purge from history | Repo / VCS (history) | Remote (GitHub) | `git filter-repo` rewrites objects; force-push republishes |
| Purge verification | Remote (fresh clone) | — | Only a clean-room re-clone proves the remote is clean |
| LICENSE distribution | Build (Makefile `DISTRIBUTABLES`) | Repo (root file) | Makefile already globs `LICENSE*` into the `.vcvplugin` |
| NOTICES / OFL attribution | Repo + Build | Distributable | OFL requires its text to travel with the shipped font |
| SVG outline provenance | Asset (res/) | Human judgment | No metadata in paths — provenance is a human decision, not a tool output |

## Standard Stack

### Core
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| `git filter-repo` | 2.47.0 (brew) | Purge font blobs from all history | Officially recommended by `git` itself over the deprecated `filter-branch`; the standard modern history-rewrite tool [CITED: git-scm.com/docs/git-filter-branch — "filter-branch ... use git filter-repo instead"] |
| `git` | 2.39.5 (installed) | Clone, force-push, verify | filter-repo needs git ≥ 2.24 and python3 ≥ 3.6 — both satisfied (python 3.14.2) [VERIFIED: `git --version`, `python3 --version`] |
| `gh` CLI | authed as `Photep` | Confirm/keep repo private; HTTPS auth for force-push | Repo confirmed PRIVATE via `gh repo view` [VERIFIED: gh repo view] |

### Supporting
| Asset | Purpose | When to Use |
|-------|---------|-------------|
| GPL-3.0 license text (`COPYING`/`gpl-3.0.txt`) | The `LICENSE` file body | plugin.json declares `GPL-3.0-or-later`; the `LICENSE` file must be the verbatim GPLv3 text [VERIFIED: plugin.json] |
| SIL OFL 1.1 text (`OFL.txt`) | Ships beside JetBrains Mono per OFL terms | OFL requires its license to accompany the redistributed font (see Don't Hand-Roll) [CITED: openfontlicense.org/ofl-faq] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `git filter-repo` | BFG Repo-Cleaner | BFG is fine for blob removal but is a separate Java tool, not installed; filter-repo is the project's natural fit and the ROADMAP success criteria name `filter-repo --invert-paths` explicitly. Use filter-repo. |
| `git filter-repo` | `git filter-branch` | Deprecated, slow, error-prone, leaves refs/original cruft. Do NOT use. |
| Install filter-repo via `brew` | `pip3 install git-filter-repo` | Both work; `brew install git-filter-repo` (2.47.0 bottled) is one command and PATH-clean. Recommend brew. |

**Installation:**
```bash
brew install git-filter-repo        # 2.47.0, verified available in brew
# fallback: pip3 install git-filter-repo
git filter-repo --version           # confirm before use
```

**Version verification:** `git-filter-repo` 2.47.0 is the current brew stable (verified via `brew info`). `git` 2.39.5 and python 3.14.2 both exceed filter-repo's minimums.

## Package Legitimacy Audit

> This phase installs exactly one external tool (`git-filter-repo`). No npm/PyPI app dependencies are added to the plugin.

| Package | Registry | Age | Downloads | Source Repo | slopcheck | Disposition |
|---------|----------|-----|-----------|-------------|-----------|-------------|
| `git-filter-repo` | Homebrew / PyPI | 8+ yrs (since 2019) | very high | github.com/newren/git-filter-repo | not run (no JS/Py app dep) | Approved — authored by `newren` (a git core contributor), endorsed in official git docs |

**Packages removed due to slopcheck [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

`git-filter-repo` is a single Python script maintained by Elijah Newren (a Git core maintainer) and is the tool the Git project's own documentation points users to. Provenance is authoritative; install from brew or PyPI. [CITED: github.com/newren/git-filter-repo]

## Architecture Patterns

### System Architecture Diagram

```
                 LOCAL REPO (main, HEAD=aa8210a)              REMOTE (origin/main=2cf7b59 == tag v1.3, PRIVATE)
                 75 commits ahead of remote                   fonts present (e486ce1 is ancestor)
                 fonts tracked at HEAD                                 |
                          |                                            |
   P1  git rm fonts ──────┤                                            |
       + .gitignore       │                                            |
       + LICENSE/NOTICES  ▼                                            |
       commit "ip-clean"  (working tree now clean)                     |
                          |                                            |
   P3  git push origin ───┼───────────── force/normal ────────────────►  remote now == local (76+ commits, fonts still in HISTORY)
       main --tags        |                                            |
                          ▼                                            |
              git clone <remote> fresh/ ◄───────── clone ──────────────┤
                          |                                            |
       git filter-repo --invert-paths                                  |
         --path BCBarellTEST-Regular.otf                               |
         --path FoundationLogo.ttf      (rewrites ALL commits + tags)  |
                          |  (filter-repo DROPS the origin remote)     |
       git remote add origin <url>                                     |
                          |                                            |
       git push --force --all  ─────────── force-push ────────────────►  remote history rewritten, blobs gone
       git push --force --tags ────────────────────────────────────────►  tag v1.3 rewritten
                          |                                            |
   VERIFY   git clone <remote> verify/ ◄───── 2nd fresh clone ─────────┤
                          |                                            |
       git rev-list --all --objects | grep -iE 'Barell|FoundationLogo' │
                  └── MUST be EMPTY  ──►  HARD GATE for Phase 28        |
                          |                                            |
   RESYNC   local working repo SHAs now diverge from rewritten remote  |
            → re-clone or hard-reset local to rewritten origin/main    |
```

### Pattern 1: Safe filter-repo purge sequence (the load-bearing recipe)
**What:** Rewrite history on a throwaway fresh clone, never on the working repo.
**When to use:** P3 only, after P1's removal commit is on the remote.
**Example:**
```bash
# 0. PRECONDITION: P1 removal commit + all 75 local commits are pushed to origin.
#    Confirm remote is PRIVATE first:
gh repo view Photep/ForgeAudio-AnalogSeries --json isPrivate   # must be true

# 1. Fresh clone into a scratch dir (NOT inside the working repo)
git clone https://github.com/Photep/ForgeAudio-AnalogSeries.git /tmp/purge-clone
cd /tmp/purge-clone

# 2. Purge both blobs from ALL commits, branches, and tags
git filter-repo --invert-paths \
  --path BCBarellTEST-Regular.otf \
  --path FoundationLogo.ttf

# 3. filter-repo REMOVES 'origin' by design — re-add it
git remote add origin https://github.com/Photep/ForgeAudio-AnalogSeries.git

# 4. Force-push rewritten history + tags
git push --force --all origin
git push --force --tags origin

# 5. VERIFY on a SECOND fresh clone (the working clone is now untrustworthy)
git clone https://github.com/Photep/ForgeAudio-AnalogSeries.git /tmp/verify-clone
cd /tmp/verify-clone
git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'   # MUST print nothing
# also confirm the blob OIDs are gone:
git cat-file -t 031e8db9c923a6071e910df0105ef245cd63aedb 2>&1   # expect: missing/error
git cat-file -t 3533f3e4cffbe400f6126f1be4dc50c88cbc4649 2>&1   # expect: missing/error
```

### Pattern 2: LICENSE auto-distribution (no Makefile change)
**What:** The Makefile already globs `LICENSE*` into the `.vcvplugin`.
**When to use:** P1 — just drop the file at root.
```make
# Already present in Makefile (verified):
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)
```
Creating `LICENSE` at repo root is sufficient for success criterion #4 — **no Makefile edit needed for LICENSE**. (A NOTICES file is a *separate* glob — see Pitfall 3.)

### Anti-Patterns to Avoid
- **Running filter-repo on the working repo:** corrupts your only copy of the 75 unpushed commits and leaves reflog/stash cruft. Always use a fresh throwaway clone.
- **Force-pushing before pushing the new local commits:** the fresh-clone-of-remote starts from v1.3 and a force-push would erase Phase 22-24 from the remote permanently. Push first, purge second.
- **Trusting the purge clone:** filter-repo's own clone still has the new (clean) objects but you must prove the *remote* is clean — verify on an independent second clone.
- **Forgetting `--tags`:** tag `v1.3` (and its `^{}` peeled commit) contains the fonts; a branches-only force-push leaves the blobs reachable via the tag.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Purging blobs from history | Manual `git rebase`/`commit --amend` chain | `git filter-repo --invert-paths` | Rebase can't reliably reach every branch/tag and won't repack/expire unreachable objects; filter-repo does it atomically [CITED: github.com/newren/git-filter-repo] |
| GPL-3.0 license body | Paraphrased/edited GPL text | Verbatim FSF GPLv3 text | GPL must be reproduced verbatim; editing it voids the grant. Fetch from gnu.org/licenses/gpl-3.0.txt |
| Attribution for the shipped font | Custom credit string only | Bundle the full `OFL.txt` + a NOTICES entry | OFL §clauses require the license text to accompany any redistribution of the font binary; a one-line credit is insufficient [CITED: openfontlicense.org/ofl-faq] |

**Key insight:** History rewriting and license compliance are both domains where "almost right" is "wrong." A purge that misses the tag, or a NOTICES file that omits the OFL text shipped beside the `.ttf`, both fail the legal goal even though they look done.

## Runtime State Inventory

> This is a purge/history-rewrite phase — the canonical question is: *after the working tree and history are clean, what still references the trial fonts or carries stale font state?*

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data (git history) | Blobs `031e8db...` (BCBarell) and `3533f3e...` (FoundationLogo); reachable from `e486ce1` through HEAD on local **and** remote, and from tag `v1.3`. [VERIFIED: `git rev-list --all --objects`] | `git filter-repo --invert-paths` (P3) — both `--all` branches and `--tags` |
| Live service config (remote) | GitHub remote `origin/main` is 75 commits BEHIND local and still PRIVATE; tags v1.0-v1.3 pushed. Only v1.3 tree contains the fonts (v1.0-v1.2 predate `e486ce1`). [VERIFIED: `git ls-remote`, per-tag `git ls-tree`] | Push local→remote BEFORE purge; force-push `--all --tags` AFTER |
| OS-registered / external | None. Fonts are not installed system-wide, not referenced by CI, not in `plugin.json`. [VERIFIED: grep of src/, Makefile, plugin.json, res/ — zero references outside docs] | None |
| Doc references (non-binary) | `DESIGN-LANGUAGE.md` names "FoundationLogo (fallback Bebas Neue)" for the brand; `CODE-REVIEW-FINDINGS.md` documents the trial-font issue. These are prose, not font binaries. [VERIFIED: grep] | No purge needed (text mentions are not redistributable font software); optionally soften DESIGN-LANGUAGE wording in P2 |
| Build artifacts | `dist/ForgeAudio-AnalogSeries/res/fonts/JetBrainsMonoNL-Regular.ttf` exists but `dist/` is gitignored — never in history. No trial font in `dist/`. [VERIFIED: `.gitignore`, `find`] | None (rebuild produces fresh dist) |
| Local repo post-rewrite | After force-push, the local working repo's SHAs diverge from the rewritten remote (this repo is where GSD commits `.planning/` docs). | Re-clone or `git reset --hard` local `main` to the rewritten `origin/main`; re-apply any uncommitted `.planning` work |

**The two blob OIDs to confirm dead after purge:** `031e8db9c923a6071e910df0105ef245cd63aedb` (BCBarellTEST), `3533f3e4cffbe400f6126f1be4dc50c88cbc4649` (FoundationLogo).

## Common Pitfalls

### Pitfall 1: The 75-commit cliff (HIGHEST operational risk)
**What goes wrong:** Following the ROADMAP recipe literally — "fresh clone of remote → filter → force-push" — clones the remote at v1.3 and force-pushes that, **erasing all Phase 22-24 work from the remote**.
**Why it happens:** Local `main` (aa8210a) is 75 commits ahead of `origin/main` (2cf7b59 == v1.3). The remote was never updated after v1.3. [VERIFIED: `git log origin/main..HEAD` = 75]
**How to avoid:** P3 step 0 = push the P1 removal commit and all local commits to origin FIRST (`git push origin main --tags`), so the fresh clone contains everything. Only then filter + force-push.
**Warning signs:** `git status` says "ahead of 'origin/main' by N commits"; a fresh clone has fewer commits than `git rev-list --count HEAD` locally.

### Pitfall 2: Self-corruption of the working repo (GSD lives here)
**What goes wrong:** GSD commits `.planning/` docs (including this RESEARCH.md) to the same repo being rewritten. After the force-push, the local repo diverges and future GSD commits would re-introduce orphaned history or fail to push.
**Why it happens:** filter-repo rewrites every SHA; the local clone still has the old SHAs.
**How to avoid:** Run filter-repo only on a throwaway clone (Pattern 1). After verification, hard-reset/re-clone the working repo to the rewritten `origin/main`. Sequence the rewrite to a moment with no uncommitted GSD work in flight, then re-establish.
**Warning signs:** `git pull` reports "divergent branches" with no common ancestor; pushes rejected as non-fast-forward after the rewrite.

### Pitfall 3: NOTICES file silently not shipped
**What goes wrong:** A `NOTICES` file is created but `make dist` doesn't include it — the OFL text never reaches the user.
**Why it happens:** `DISTRIBUTABLES` globs `res`, `LICENSE*`, and `presets` only — **`NOTICES` matches none of these.** [VERIFIED: Makefile]
**How to avoid:** Either add `DISTRIBUTABLES += $(wildcard NOTICES*)` to the Makefile, OR place the credits + `OFL.txt` under `res/` (already a distributable). Recommend: ship `res/fonts/OFL.txt` beside the font (OFL's intent) AND add `NOTICES` to `DISTRIBUTABLES` for the human-readable credit.
**Warning signs:** Unzip the `.vcvplugin` and the OFL text is absent next to `JetBrainsMonoNL-Regular.ttf`.

### Pitfall 4: Wordmark outline provenance (the real IP-03 risk)
**What goes wrong:** The shipped `res/AnalogLFO.svg` FORGE/AUDIO wordmark outlines may have been drawn from the **trial FoundationLogo** font (DESIGN-LANGUAGE lists it as the primary brand face). Path geometry carries no font metadata, so a clean SVG (no embedded font) can still embody a trial font's *design*.
**Why it happens:** Outlines are just Bézier paths; there is no way to read "which font made this" from the SVG. [VERIFIED: SVG has 0 glyph/font/text nodes, 120 paths]
**How to avoid:** Human-confirm (P2) that the FORGE/AUDIO and "ANALOG LFO" outlines were generated from the **OFL fonts** (Bebas Neue / Chakra Petch), not FoundationLogo. If uncertain, regenerate those specific wordmark paths from a confirmed-OFL font. Bebas Neue and Chakra Petch are both OFL 1.1 and OFL permits outline embedding (see State of the Art). [CITED: fonts.google.com/specimen/Bebas+Neue, /Chakra+Petch]
**Warning signs:** No record of which tool/font produced the wordmark paths; the only documented brand font is the trial FoundationLogo.

### Pitfall 5: Re-clone uses cached/SSH host that bypasses the rewrite
**What goes wrong:** Verifying against a stale local object cache or the purge-clone itself instead of a true fresh remote clone gives a false "clean" pass.
**How to avoid:** Verify in a brand-new directory cloned fresh from the HTTPS remote URL; never reuse the filter-repo working clone for verification.

## Code Examples

### Verify the purge is complete (the Phase 28 hard gate)
```bash
# Run inside a SECOND, independent fresh clone of the remote:
git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'
# Expected output: (empty) — exit code 1 from grep means PASS

# Belt-and-suspenders: confirm the specific blob OIDs are unreachable
for oid in 031e8db9c923a6071e910df0105ef245cd63aedb \
           3533f3e4cffbe400f6126f1be4dc50c88cbc4649; do
  git cat-file -e "$oid" 2>/dev/null && echo "STILL PRESENT: $oid" || echo "GONE: $oid"
done
```

### .gitignore additions (IP-01)
```gitignore
# Trial/mockup-only fonts — never redistribute (purged from history Phase 25)
BCBarellTEST-Regular.otf
FoundationLogo.ttf
```

### IP-01 removal commit
```bash
git rm BCBarellTEST-Regular.otf FoundationLogo.ttf
# append the .gitignore block above
git add .gitignore LICENSE NOTICES res/fonts/OFL.txt
git commit -m "chore(25): remove trial fonts, add GPL LICENSE + OFL NOTICES (IP-01,PKG-01,PKG-04)"
make && make dist     # confirm build unaffected; LICENSE lands in dist, no trial fonts
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `git filter-branch` | `git filter-repo` | git ~2.24 (2019) | filter-branch is officially discouraged in git's own docs; filter-repo is the standard |
| Ambiguity on OFL + outlines | OFL-FAQ explicitly permits embedding incl. converting glyphs to outlines | OFL 1.1 / current FAQ | Bebas Neue & Chakra Petch outlines baked into the panel are redistributable under GPL; OFL restrictions do not propagate to the embedded document outlines [CITED: openfontlicense.org/ofl-faq] |

**Deprecated/outdated:**
- `git filter-branch`: do not use for this purge.

**OFL outline finding (HIGH confidence for the OFL fonts):** The OFL-FAQ states that embedding an OFL font in a document — including converting characters to outlines/paths — is permitted, and the embedded outlines are not subject to the OFL's redistribution restrictions. Therefore Bebas Neue (OFL 1.1) and Chakra Petch (OFL 1.1) outlines in `res/AnalogLFO.svg` are safe to ship under GPL-3.0. The residual risk is solely whether any wordmark outline came from the **non-OFL FoundationLogo trial** font (Pitfall 4 / Assumption A2).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The FORGE/AUDIO + "ANALOG LFO" wordmark outlines in `res/AnalogLFO.svg` were generated from OFL fonts (Bebas Neue / Chakra Petch), not the trial FoundationLogo | Pitfall 4 | If drawn from FoundationLogo, the shipped panel art derives from a trial font — must regenerate those paths before public release. **HIGH-STAKES — human confirm in P2.** |
| A2 | "Bebas Neue" used in the panel is the Google Fonts / Dharma Type OFL cut, not the older freeware/commercial Bebas | State of the Art | A non-OFL Bebas cut would change the provenance conclusion. Confirm the source was Google Fonts. |
| A3 | The 120-path SVG has no externally-referenced or `<use>`-linked glyph defs hiding font data | SVG census | Verified 0 `<text>/<glyph>/<font>/@font-face/<image>/base64` — low risk, but confirm no `<use href>` pulls in a font symbol |
| A4 | Pushing the 75 local commits to the private remote before the public flip is acceptable (they will be purged-then-republished, repo stays private throughout) | Pitfall 1 | If the operator wants the remote to stay at v1.3 until release, the purge sequence must instead run on the local repo with extra care. Confirm push-first is acceptable. |

## Open Questions

1. **Wordmark outline provenance (A1) — the phase's defining risk.**
   - What we know: shipped SVG is 100% vector paths, no font binary ships; Bebas Neue & Chakra Petch are OFL and safe; FoundationLogo is the trial font being purged and is named as the brand face in DESIGN-LANGUAGE.md.
   - What's unclear: whether the FORGE/AUDIO outlines were rendered from FoundationLogo or from the Bebas Neue fallback.
   - Recommendation: P2 is a human-gated provenance confirmation. If the operator can't confirm Bebas/Chakra origin, regenerate the wordmark paths from a known-OFL font and re-export the SVG.

2. **Push-first vs. rewrite-local (A4).**
   - What we know: local is 75 commits ahead; remote is at v1.3.
   - Recommendation: push-first (Pattern 1). Repo stays private the entire time, so pushing pre-purge history to a private remote is not a disclosure.

3. **Where to place OFL.txt + NOTICES so `make dist` ships them (Pitfall 3).**
   - Recommendation: `res/fonts/OFL.txt` (auto-shipped via `res`) + root `NOTICES` with `DISTRIBUTABLES += $(wildcard NOTICES*)`.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `git` | All plans | ✓ | 2.39.5 | — |
| `git-filter-repo` | P3 purge | ✗ (not yet installed) | brew has 2.47.0 | `pip3 install git-filter-repo` |
| `gh` CLI (authed) | Confirm private, HTTPS push | ✓ | logged in as Photep | PAT over HTTPS |
| `python3` | filter-repo runtime | ✓ | 3.14.2 | — |
| `brew` | install filter-repo | ✓ | present | pip3 |
| `../Rack-SDK` | `make`/`make dist` sanity check in P1 | (per project workflow) | — | skip dist check, rely on `make test` |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** `git-filter-repo` — install via brew (preferred) or pip3 before P3.

## Validation Architecture

> nyquist_validation key absent in config → treated as enabled. This phase's "tests" are deterministic shell-command gates, not unit tests; the existing doctest suite must remain green because LICENSE/gitignore changes don't touch code.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | doctest 2.4.11 (vendored) — existing harness |
| Config file | Makefile `test` target (Rack-free) |
| Quick run command | `make test` |
| Full suite command | `make test` (47 cases as of Phase 24) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| IP-01 | Trial fonts gone from working tree + gitignored | smoke | `test -e BCBarellTEST-Regular.otf && echo FAIL \|\| echo OK; git check-ignore BCBarellTEST-Regular.otf FoundationLogo.ttf` | ✅ shell |
| IP-02 | Fonts purged from ALL history on remote | smoke (clean-room) | `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` (empty = pass) in a fresh clone | ✅ shell |
| IP-03 | Panel outline provenance acceptable | manual-only | human confirmation (A1) — no automated check possible | ❌ human gate |
| PKG-01 | GPL-3.0 LICENSE at root + in dist | smoke | `test -f LICENSE && unzip -l dist/*.vcvplugin \| grep -q LICENSE` | ✅ shell |
| PKG-04 | NOTICES inventories third-party assets + OFL ships | smoke | `test -f NOTICES && unzip -l dist/*.vcvplugin \| grep -qi OFL` | ✅ shell |
| (regression) | Code untouched, suite still green | unit | `make test` | ✅ exists |

### Sampling Rate
- **Per task commit:** `make test` (must stay 47/47 green — these changes are non-code).
- **Per wave merge:** the IP-01/PKG smoke commands above.
- **Phase gate:** IP-02 clean-room grep returns empty on an independent fresh clone — this is the hard gate Phase 28 depends on.

### Wave 0 Gaps
- None for unit tests — no code changes, existing harness suffices.
- Add a short `scripts/verify-purge.sh` (optional) wrapping the IP-02 verification grep + blob-OID check for repeatable gating.

## Security Domain

> security_enforcement key absent → treated as enabled. This is a docs/IP/VCS phase with no application attack surface; most ASVS categories are N/A. The security concerns here are *operational* (irreversible history rewrite, disclosure timing).

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | n/a — no auth code |
| V3 Session Management | no | n/a |
| V4 Access Control | partial | GitHub repo MUST remain PRIVATE until Phase 28 purge-verify passes (PUB-01 gate) |
| V5 Input Validation | no | n/a |
| V6 Cryptography | no | n/a |
| V10/V14 Supply chain | yes | install `git-filter-repo` only from brew/PyPI (authoritative, see audit) |

### Known Threat Patterns for this phase
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Premature public exposure of trial fonts | Information Disclosure | Keep repo PRIVATE through the entire purge; flip public only after the clean-room grep passes (Phase 28) |
| Irreversible history loss (75 commits) | Denial / data loss | Push local→remote before purge; run filter-repo on a throwaway clone; keep the pre-rewrite remote reflog until verification passes |
| Wrong-repo force-push | Tampering | filter-repo strips `origin` by design; re-add the exact verified URL; confirm `gh repo view` before pushing |
| Token leakage in HTTPS push | Info disclosure | Use `gh`-managed credential helper (already configured); never embed the PAT in commands/logs |

## Project Constraints (from CLAUDE.md)

No `./CLAUDE.md` exists in the repo (checked — absent). No `.claude/skills/` or `.agents/skills/` directory exists. Project conventions instead come from `MEMORY.md` (auto-memory): the build/install workflow uses a relative `../Rack-SDK` with a stale-install flush and a built-vs-installed shasum match — relevant only to the P1 `make`/`make dist` sanity check, not to the purge.

## Sources

### Primary (HIGH confidence)
- Direct repo inspection (`git ls-files`, `git rev-list --all --objects`, `git ls-remote`, `git log origin/main..HEAD`, per-tag `git ls-tree`, SVG element census, Makefile, plugin.json, .gitignore) — 2026-06-30
- `gh repo view Photep/ForgeAudio-AnalogSeries` — confirmed PRIVATE
- git-scm.com — `git filter-branch` docs recommend `git filter-repo`
- github.com/newren/git-filter-repo — tool provenance + `--invert-paths`/`--path` usage
- openfontlicense.org/ofl-faq — embedding + outline conversion permitted under OFL
- fonts.google.com/specimen/Bebas+Neue, /Chakra+Petch — both OFL 1.1

### Secondary (MEDIUM confidence)
- en.wikipedia.org/wiki/SIL_Open_Font_License — OFL summary (cross-check)
- CODE-REVIEW-FINDINGS.md (project) — original IP issue #5/#7 framing, "JetBrains Mono is OFL — fine to ship"

### Tertiary (LOW confidence)
- None relied upon for decisions.

## Metadata

**Confidence breakdown:**
- Repo state / purge mechanics: HIGH — verified directly against the live repo and authoritative git docs.
- LICENSE/NOTICES distribution: HIGH — Makefile globs verified; OFL requirement cited.
- OFL-font outline legality: HIGH — OFL-FAQ explicit; both fonts confirmed OFL 1.1.
- Wordmark provenance (FoundationLogo vs Bebas): LOW — undeterminable from paths; **human gate required (A1)**.

**Research date:** 2026-06-30
**Valid until:** 2026-07-30 (stable domain; re-verify the 75-commit divergence and remote tip immediately before P3 — it may change if commits are pushed in the interim)

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| IP-01 | Trial fonts removed from working tree + gitignored | Both at repo root, tracked; exact `git rm` + `.gitignore` block provided (Code Examples) |
| IP-02 | Fonts purged from full history via filter-repo, force-pushed to private remote, verified via fresh clone | Full safe sequence (Pattern 1) accounting for the 75-commit divergence; blob OIDs + verification grep provided |
| IP-03 | SVG/panel art font-outline + asset provenance confirmed acceptable for GPL | SVG is path-only (no font ships); OFL outline legality confirmed; FoundationLogo wordmark provenance flagged as the one human-gated risk (A1) |
| PKG-01 | GPL-3.0 LICENSE at repo root, picked up by DISTRIBUTABLES | Makefile already globs `LICENSE*` — no Makefile change; drop verbatim GPLv3 text |
| PKG-04 | NOTICES inventories third-party asset licenses | JetBrains Mono NL = OFL (ship `OFL.txt`); Bebas Neue + Chakra Petch outlines = OFL embedding (credit in NOTICES); NOTICES needs its own DISTRIBUTABLES glob or placement under res/ (Pitfall 3) |
