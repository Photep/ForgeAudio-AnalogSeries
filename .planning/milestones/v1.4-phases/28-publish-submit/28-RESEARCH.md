# Phase 28: Publish + Submit - Research

**Researched:** 2026-07-10
**Domain:** Release operations — git tagging, GitHub repo visibility flip, VCV Library issue-based submission
**Confidence:** HIGH (submission mechanism verified against the authoritative VCVRack/library README + current 2026 issue activity; git/gh state verified locally)

## Summary

Phase 28 is a **pure operations phase** — no code changes. It performs two outward-facing, effectively-irreversible actions in a strict order: (1) flip the currently-PRIVATE GitHub repo to PUBLIC, and (2) open exactly one submission issue on `github.com/VCVRack/library`. The entire value of this research is nailing the exact mechanism, the required issue contents, and the safe ordering so the private→public exposure never precedes a clean-history re-verification.

The VCV Library submission process is **issue-based and unchanged in 2026** (verified: active submissions dated July 2026). You create **exactly one** issue on `VCVRack/library`, titled with your **plugin slug** (`ForgeAudio-AnalogSeries`, not the display name), and post a comment carrying the plugin name, license, URLs, version, and — critically — the **exact commit hash** (`git rev-parse HEAD`, never a branch name). VCV's own build system (`rack-plugin-toolchain`) then clones your public `sourceUrl` at that commit and builds all platforms itself. This means: the commit must be **pushed and publicly reachable**, a git **tag is good practice but a GitHub Release is NOT required**, and you never attach a `.vcvplugin` for them to use.

The dominant risk is ordering. The Phase 25 font-history purge is a **hard gate (IP-02/PUB-01)**: history was already purged + verified clean while private, but PUB-01 mandates a **final fresh-clone grep immediately before the flip**. If the flip preceded verification, any reintroduced blob would already be public — defeating the gate. Safe order: finalize tree + tag + push (while private) → re-verify remote clean on a `--mirror` clone → flip public → confirm public reachability → open the one issue.

**Primary recommendation:** Merge the release tree to `main`, tag `v2.0.0`, push (private) → run the `--mirror` purge re-verify as a HARD STOP gate → `gh repo edit --visibility public --accept-visibility-change-consequences` (human-gated) → verify anonymous reachability → open ONE `VCVRack/library` issue titled `ForgeAudio-AnalogSeries` with name/license/URLs/version `2.0.0`/full commit hash → record the issue URL and hash to STATE.md.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PUB-01 | GitHub repo flipped to public ONLY after IP-02 purge verification passes; final fresh-clone grep confirms history clean | "The Public-Flip Safety Gate" section — exact `git clone --mirror` + `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` HARD-STOP command, plus specific-OID absence check (031e8db, 3533f3e), plus safe ordering (verify-while-private → flip) |
| PUB-02 | VCV Library submission issue opened — titled with plugin slug, containing sourceUrl and exact commit hash | "VCV Library Submission Mechanism" section — verbatim README requirements, exact `gh issue create` command, title = slug `ForgeAudio-AnalogSeries`, body fields, commit-hash-not-branch rule |
</phase_requirements>

## User Constraints

**No CONTEXT.md exists** for Phase 28 at research time (`/gsd:discuss-phase` not yet run). The binding constraints therefore come from ROADMAP success criteria, REQUIREMENTS (PUB-01, PUB-02), and prior-phase locked decisions:

- **Slug is permanent and locked:** plugin `ForgeAudio-AnalogSeries`, module `ForgeAnalogLFO` (PKG-03). Do not rename.
- **Version is locked at `2.0.0`** — MAJOR = Rack major (2), NOT relabeled to 1.4.x. Tag must be `v2.0.0` (ROADMAP explicitly says `v2.x.y`). [CITED: REQUIREMENTS Out of Scope + PKG-03]
- **License locked:** `GPL-3.0-or-later` (from plugin.json).
- **IP-02 purge gate is CLEARED but must be RE-VERIFIED at flip time** (STATE.md Blockers: "re-confirm EMPTY on a final fresh clone at flip time — PUB-01").
- **Exactly one submission issue** — permanent channel; all future updates are comments on it (ROADMAP criterion 4).

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| History-clean re-verification | Local git (clean-room clone) | GitHub remote | Verification must read the *remote's* full object graph, not the working tree; a `--mirror` clone is the only exhaustive source |
| Release tagging | Local git → GitHub remote | — | Tag is created locally, pushed to remote; VCV reads the commit, not the tag object |
| Visibility flip | GitHub (repo settings) via `gh` | — | Server-side setting; only GitHub can expose the repo |
| Plugin build | VCV build infrastructure (`rack-plugin-toolchain`) | — | VCV clones public sourceUrl@commit and builds all platforms — NOT the submitter's machine |
| Submission channel | GitHub issue on `VCVRack/library` | — | The one issue is the permanent comms channel with the Library team |

## Standard Stack

This is an operations phase — the "stack" is CLI tooling, all verified present locally.

### Core
| Tool | Version (verified) | Purpose | Why Standard |
|------|--------------------|---------|--------------|
| `git` | 2.39.5 (Apple) | Tag, push, mirror-clone verification | Native VCS; `rev-parse`/`rev-list` are the canonical hash + history tools |
| `gh` | 2.83.2 (2025-12-10) | Flip visibility, open the submission issue non-interactively | Official GitHub CLI; only tool that scripts visibility change + issue creation |
| `git-filter-repo` | present (a40bce54) | (Reference only — the purge already ran in Phase 25; not re-run here) | Same tool used for the Phase 25 purge; available if a re-purge were ever needed |
| `curl` | system | Unauthenticated public-reachability probes post-flip | Confirms 200s without a token (true public visibility) |

**All four verified available.** `gh` is authed as `Photep` (active account, keyring token). No installs required.

### Environment facts (verified 2026-07-10)
| Fact | Value | Implication |
|------|-------|-------------|
| Repo visibility | **PRIVATE** | Flip is still pending — PUB-01 gate is live |
| Remote | `https://github.com/Photep/ForgeAudio-AnalogSeries.git` | Matches sourceUrl in plugin.json |
| Current branch | `chore/finalise-18hp-docs-and-cleanup` | **Release work is NOT on `main`** — see Pitfall 1 |
| HEAD | `20e47ca8b0b45942d193407da6b275963ddc9a2c` | Candidate release commit (on the feature branch) |
| Existing tags | `v1.0 v1.1 v1.2 v1.3` | **No `v2.0.0` yet** — must be created |
| Working tree | clean | No uncommitted work to lose |
| plugin.json version | `2.0.0` | Tag `v2.0.0`; submit version `2.0.0` (no `v` prefix in the manifest field) |
| Makefile VERSION | **none** — no `VERSION` line | Version is read from `plugin.json` by Rack's `plugin.mk`; no Makefile edit needed (see State of the Art) |

## Package Legitimacy Audit

**N/A — this phase installs no external packages.** All tooling (`git`, `gh`, `curl`, `git-filter-repo`) is pre-installed and verified above. No registry (npm/PyPI/crates) interaction occurs. slopcheck not applicable.

## VCV Library Submission Mechanism (PUB-02)

**Source of truth:** `github.com/VCVRack/library` `README.md` (fetched verbatim 2026-07-10) [VERIFIED: raw.githubusercontent.com/VCVRack/library/master/README.md]. Process confirmed still current — open issues dated July 2026 (NTRWabot 2026-07-07, LuxCache 2026-06-28) [VERIFIED: WebFetch of VCVRack/library/issues].

### The exact process (verbatim intent from the README)

1. **Create exactly ONE thread** in the Issue Tracker (`https://github.com/VCVRack/library/issues`), **with a title equal to your plugin slug**. → Title: **`ForgeAudio-AnalogSeries`** (the plugin `slug`, NOT the display name "Forge Audio - Analog Series", NOT the module slug `ForgeAnalogLFO`). This thread is your **permanent communication channel**.

2. **Post a comment** (the issue body serves as the first comment) containing:
   - **Plugin name** — `Forge Audio - Analog Series`
   - **License** — `GPL-3.0-or-later` (SPDX identifier; README: use the SPDX abbreviation)
   - **All relevant URLs** — `sourceUrl` + `manualUrl` (from plugin.json)
   - **Email address** — *only if you want it public* (optional — operator decision)
   - For the build: **the new version** (`2.0.0`) **and the commit hash**

3. **Commit hash rule (load-bearing):** the README states the hash must come from `git log` or `git rev-parse HEAD`, and *"Please do not just give the name of a branch like `master`."* → Submit the **full 40-char hash**, never `main`/`v2.0.0`.

### There is no enforced issue template or label

The README documents no template, labels, or bot automation. A Library/Review team member handles the request manually and comments when the plugin is added. Do not depend on automation.

### Exact command (planner turns into a task — HUMAN-GATED)

```bash
# Capture the exact hash of the pushed, public release commit
COMMIT=$(git rev-parse v2.0.0^{commit})   # dereference the tag to its commit
echo "$COMMIT"

gh issue create --repo VCVRack/library \
  --title "ForgeAudio-AnalogSeries" \
  --body "$(cat <<EOF
**Plugin name:** Forge Audio - Analog Series
**Slug:** ForgeAudio-AnalogSeries
**License:** GPL-3.0-or-later
**Version:** 2.0.0
**Source URL:** https://github.com/Photep/ForgeAudio-AnalogSeries
**Manual URL:** https://github.com/Photep/ForgeAudio-AnalogSeries/blob/main/docs/index.md
**Commit hash:** ${COMMIT}

Open-source plugin (GPL-3.0-or-later). Please build from the commit hash above.
EOF
)"
```

`gh issue create` prints the created issue URL on success — **capture and record it** (ROADMAP criterion 4; all future version updates are comments on this same issue).

### What happens after submission

VCV's `rack-plugin-toolchain` clones the public `sourceUrl` at the submitted commit and builds Mac/Windows/Linux itself; if integration tests pass, the plugin is added to the Plugin Manager [VERIFIED: WebSearch — "built by the official Rack build system using the rack-plugin-toolchain"]. Review is manual and may take days. No further action needed unless the team requests changes in-thread.

## The Public-Flip Safety Gate (PUB-01)

### Order of operations (safest — reason: never expose unverified history)

```
1. Finalize release tree on main  →  2. Tag v2.0.0 + push (PRIVATE)  →
3. PUB-01 re-verify remote CLEAN (HARD STOP)  →  4. Flip PUBLIC (human gate)  →
5. Verify anonymous reachability  →  6. Open the ONE issue  →  7. Record hash + issue URL
```

**Why verify AFTER tag/push but BEFORE flip:** verification must read the exact remote state that is about to become public — including the new `v2.0.0` tag. Verifying a `--mirror` clone *after* pushing the tag but *before* the flip guarantees no ref (branch or tag) carries a purged blob at the moment of exposure. Flipping first would expose history before the check — defeating the gate entirely.

### Step 3 — the HARD-STOP re-verify (while still PRIVATE)

```bash
set -euo pipefail
TMP=$(mktemp -d)
# --mirror pulls ALL refs (every branch + every tag), so rev-list --all is exhaustive
git clone --mirror https://github.com/Photep/ForgeAudio-AnalogSeries.git "$TMP/verify.git"
cd "$TMP/verify.git"

# Primary check: no object path in ANY ref matches the trial-font names
if git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'; then
  echo "DIRTY — trial-font object found in history. ABORT FLIP."; exit 1
else
  echo "CLEAN — no trial-font paths in any ref."
fi

# Belt-and-braces: the two known trial-font blob OIDs must be ABSENT
for OID in 031e8db 3533f3e; do
  if git cat-file -e "$OID" 2>/dev/null; then
    echo "DIRTY — blob $OID present. ABORT FLIP."; exit 1
  fi
done
echo "CLEAN — known trial-font blobs 031e8db / 3533f3e absent. Safe to flip."
```

- `git clone --mirror` is required (a plain `git clone` only makes the default branch a local ref, so `rev-list --all` would miss other branches). [ASSUMED — git semantics; verify empirically in the plan]
- The blob OIDs `031e8db` and `3533f3e` are the exact trial-font blobs purged in Phase 25 [CITED: STATE.md 25-03 blocker entry]. Checking their absence by OID catches a reintroduction even under a renamed path.
- **Non-empty grep = non-zero exit = HARD STOP.** The planner must model this as a blocking gate: if DIRTY, do NOT flip; escalate to a re-purge (out of scope for this phase).

### Step 4 — flip to public (IRREVERSIBLE EXPOSURE — human-gated)

```bash
gh repo edit Photep/ForgeAudio-AnalogSeries \
  --visibility public \
  --accept-visibility-change-consequences
```

- The `--accept-visibility-change-consequences` flag is **required** to change visibility non-interactively in modern `gh`; without it the command prompts and would stall in an automated context. [ASSUMED — gh 2.83 behavior; planner should confirm the exact flag name with `gh repo edit --help` in a Wave-0 task]
- **Irreversibility caveat:** you *can* flip back to private, but any content served while public may have been cloned/cached/forked in the interval. Treat the flip as a one-way door. This is the single most consequential action in the milestone — model it as a `checkpoint:human-verify` requiring explicit operator go/no-go, immediately preceded by the Step 3 CLEAN result.

### Step 5 — confirm true public reachability (unauthenticated)

```bash
# 200 unauthenticated => genuinely public (not just visible to the authed token)
curl -sI -o /dev/null -w 'repo:   %{http_code}\n' https://github.com/Photep/ForgeAudio-AnalogSeries
curl -sI -o /dev/null -w 'manual: %{http_code}\n' https://raw.githubusercontent.com/Photep/ForgeAudio-AnalogSeries/main/docs/index.md
# Optional stronger proof: anonymous clone in a token-free environment
GIT_TERMINAL_PROMPT=0 git clone https://github.com/Photep/ForgeAudio-AnalogSeries.git "$TMP/anon" && echo "anon clone OK"
```

`curl` uses no credentials, so a `200` proves genuine public access (an authed `gh`/`git` clone would succeed even while private and give a false positive). This also closes DOC-03 (manualUrl public reachability).

## Release Tagging

### What VCV actually builds from
VCV builds from the **commit hash you submit**, cloning your public repo at that commit. The git **tag** and any **GitHub Release** are for *your* record and reproducibility — **VCV requires neither**; it only needs the hash to be reachable in the public repo. [VERIFIED: README "commit hash… do not just give the name of a branch"; WebSearch build-toolchain confirmation]

### Recommended tagging

```bash
# On the finalized release commit (on main — see Pitfall 1)
git tag -a v2.0.0 -m "Forge Audio Analog Series v2.0.0 — first VCV Library release"
git push origin v2.0.0
git rev-parse v2.0.0^{commit}   # the exact 40-char hash to submit and to record
```

- Tag name **`v2.0.0`** matches plugin.json `version: 2.0.0` and ROADMAP's `v2.x.y` (the milestone label "v1.4 Tempered" is internal; the *plugin* version is `2.0.0` because VCV requires MAJOR = Rack major). Do NOT tag `v1.4`.
- **Annotated tag (`-a`) recommended** over a lightweight tag (carries author/date/message). A full **GitHub Release object is OPTIONAL** — create one only if you want public release notes; do NOT attach the `.vcvplugin` expecting VCV to use it (they rebuild from source for all platforms).
- Submit the commit hash via `v2.0.0^{commit}` dereference (an annotated tag's own SHA is the tag object, not the commit — always dereference).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| History-clean check | Custom tree-walk / working-tree grep | `git clone --mirror` + `git rev-list --all --objects \| grep` | Working-tree grep misses *history*; only a mirror + rev-list sees every blob in every ref |
| Visibility flip | GitHub web UI clicks (unlogged) | `gh repo edit --visibility public` | Scriptable, auditable, records exactly what ran; UI clicks leave no plan artifact |
| Cross-platform build for submission | Building `.vcvplugin` per-OS yourself | Submit the commit hash; VCV's `rack-plugin-toolchain` builds all platforms | VCV's build is the canonical one; local builds aren't used by the Library |
| "Is it really public?" | Trusting the authed `gh` clone | Unauthenticated `curl -I` → 200 | Authed clone succeeds even while private (false positive) |

**Key insight:** every step here has a canonical `git`/`gh` primitive. The failure mode in release ops is not missing tools — it's *ordering* and *false-positive verification* (checking with credentials that mask the private state).

## Common Pitfalls

### Pitfall 1: Release commit is on a feature branch, not `main`
**What goes wrong:** HEAD is `20e47ca` on `chore/finalise-18hp-docs-and-cleanup`; `main` exists locally and on the remote but the docs/panel finalization work sits on the feature branch. If you tag/submit the feature-branch commit, `manualUrl` (`.../blob/main/docs/index.md`) may 404 (docs not on `main`) and the submitted commit may not be on the default branch users browse.
**Why it happens:** post-purge resync left the operator on a working branch; the release-readiness commits accumulated there.
**How to avoid:** Merge/fast-forward the feature branch into `main` and push `main` **before** tagging. Tag `v2.0.0` on the `main` commit. Confirm `docs/index.md` and `docs/changelog.md` exist on `main` (manualUrl/changelogUrl point at `/blob/main/...`).
**Warning signs:** `git branch --show-current` ≠ `main` at tag time; `manualUrl` `curl` returns 404 after the flip.

### Pitfall 2: Submitting a branch name or tag name instead of the commit hash
**What goes wrong:** VCV explicitly rejects `main`/`master`/tag names; the build can't pin a reproducible source.
**How to avoid:** Always `git rev-parse v2.0.0^{commit}` and paste the full 40-char hash into the issue.
**Warning signs:** issue body contains `main`, `HEAD`, or `v2.0.0` where the hash should be.

### Pitfall 3: Opening more than one issue
**What goes wrong:** The README mandates **exactly one** thread as the permanent channel; a second issue fragments the review history and annoys the volunteer team.
**How to avoid:** Open one issue titled `ForgeAudio-AnalogSeries`. Record its URL. All future version bumps = **comments on that same issue** (new version + new commit hash). Never open a second.
**Warning signs:** the urge to "open a fresh issue" for a later update.

### Pitfall 4: Flipping public before the final purge re-verify
**What goes wrong:** exposes history before confirming it's clean — the exact failure PUB-01 exists to prevent. Once public, a leaked trial-font blob may already be cloned/forked.
**How to avoid:** enforce the ordering — Step 3 CLEAN result is a mandatory precondition of Step 4. Model as a hard gate.
**Warning signs:** any plan task that runs `gh repo edit --visibility public` before the mirror-clone grep task.

### Pitfall 5: False-positive reachability with a logged-in token
**What goes wrong:** an authed `git clone` / `gh` succeeds while the repo is still private, so "clone worked" doesn't prove public.
**How to avoid:** verify with unauthenticated `curl -I` (200) or a token-free clone.
**Warning signs:** verifying visibility with the same credentials that can see private repos.

### Pitfall 6: Title uses display name or module slug
**What goes wrong:** VCV keys the thread on the plugin slug; wrong title complicates their tooling and lookups.
**How to avoid:** Title must be **exactly `ForgeAudio-AnalogSeries`** (plugin `slug`), not "Forge Audio - Analog Series" and not `ForgeAnalogLFO`.

### Pitfall 7: Slug immutability assumed but not final
**What goes wrong:** slug is permanent once published; a typo or later regret is unfixable without a new plugin entry.
**How to avoid:** slug already locked in PKG-03 (`ForgeAudio-AnalogSeries` / module `ForgeAnalogLFO`); do a final read of plugin.json before submitting — no edits, just confirm.

## Runtime State Inventory

> This is a publish phase, not a rename/refactor. Included for completeness because irreversible external state IS created.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — no datastores touched. | None. |
| Live service config | **GitHub repo visibility** flips PRIVATE→PUBLIC (server-side, GitHub). **VCVRack/library issue** created (external repo, permanent). | Both are the deliverables; both human-gated. Record issue URL. |
| OS-registered state | None. | None. |
| Secrets/env vars | `gh` token in keyring (account `Photep`) — used, not modified. **Verify no secrets exist in the tree before flip** (history purge covered trial fonts; a `git rev-list`/secret-scan of the tree is prudent but the milestone scoped IP to fonts). | None to change; optional secret scan (see Open Questions). |
| Build artifacts | `.vcvplugin` from Phase 26 — NOT submitted (VCV rebuilds). | None — do not attach to submission. |

**The canonical question — after flip, what external systems hold state:** (1) GitHub serves the repo publicly (one-way door); (2) the `VCVRack/library` issue is a permanent public thread. Both are intended and recorded.

## Code Examples

None — this phase contains no source code. All executable artifacts are the shell/git/gh commands in the "Public-Flip Safety Gate," "Release Tagging," and "VCV Library Submission Mechanism" sections above.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Version in **Makefile `VERSION`** (README wording, v0.6 era) | Version in **`plugin.json` `version`**; `plugin.mk` reads it | Rack v2 | No Makefile edit needed — this repo has no `VERSION` line; `2.0.0` in plugin.json is authoritative [VERIFIED: WebSearch "increment the version in your plugin.json manifest file"; local Makefile grep found no VERSION] |
| Curators hand-maintain `manifests/YourSlug.json` in the library repo | Still the model, but submitter only supplies name/license/URLs/hash in the issue; team writes the manifest | ongoing | You do NOT PR a manifest file — you comment the info; the team creates `manifests/ForgeAudio-AnalogSeries.json` |
| — | Builds via `rack-plugin-toolchain` from your commit | v2 | You don't build for them; ensure the commit builds cleanly from a fresh clone |

**Deprecated/outdated:** The README's `0.6.x` version examples and "Makefile VERSION" phrasing predate Rack 2; the *process* (one issue, slug title, commit hash) is current and confirmed by 2026 issue activity.

## Validation Architecture

> No test framework applies — validation here is command-based verification of operational outcomes, not unit tests. (`workflow.nyquist_validation` absent from config; there is no code to sample.)

### Phase Requirements → Verification Map
| Req | Behavior | Verification (automated command) |
|-----|----------|-----------------------------------|
| PUB-01 | History clean before flip | `git clone --mirror` + `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` returns empty AND blobs `031e8db`/`3533f3e` absent |
| PUB-01 | Repo actually public | `curl -sI` repo URL → `200` (unauthenticated) |
| DOC-03 | Manual publicly reachable | `curl -sI` raw `docs/index.md` → `200` |
| (criterion 2) | Tag + hash recorded | `git rev-parse v2.0.0^{commit}` non-empty; hash written to STATE.md |
| PUB-02 | One issue, correct title + contents | `gh issue list --repo VCVRack/library --search "ForgeAudio-AnalogSeries in:title"` shows exactly one; body contains full commit hash (not a branch) |
| (criterion 4) | Issue URL recorded | Issue URL from `gh issue create` output written to STATE.md |

### Gate
- **Phase gate:** Step 3 mirror-grep CLEAN is a hard precondition for the flip; the flip and the issue creation are `checkpoint:human-verify` (irreversible/outward-facing).

## Security Domain

This phase writes no code and installs nothing, so most ASVS categories are N/A. The real security surface is **information exposure on the private→public flip**.

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — (no auth code) |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | no | — (no user input processed) |
| V6 Cryptography | no | — (no secrets handled in code) |
| V14 Data Protection / Info exposure | **yes** | Pre-flip history + secret scan; the PUB-01 mirror-grep gate |

### Threat patterns for a public-flip
| Pattern | STRIDE | Mitigation |
|---------|--------|------------|
| Trial-font blob still in history → IP/legal exposure | Information Disclosure | PUB-01 mirror-grep HARD STOP (primary control) |
| Leaked secret/token/key committed anywhere in history | Information Disclosure | Optional `gh` secret-scanning / `git log -p \| grep`-style scan before flip (see Open Questions) — milestone scoped IP to fonts, but flip is the last chance to catch anything else |
| Irreversible over-exposure (flip can't un-clone) | — | Human go/no-go gate immediately after CLEAN verify |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `git clone --mirror` is required for `rev-list --all` to see every ref (plain clone would miss non-default branches) | Public-Flip Safety Gate | LOW — if wrong, mirror is still safe/superset; verify empirically in plan |
| A2 | `gh repo edit --visibility public` needs `--accept-visibility-change-consequences` to run non-interactively in gh 2.83 | Public-Flip Safety Gate | MEDIUM — if flag name differs, command stalls on a prompt; confirm with `gh repo edit --help` in a Wave-0 task |
| A3 | A GitHub Release object is NOT required by VCV (only the reachable commit hash) | Release Tagging | LOW — README only asks for the hash; a Release is strictly additive |
| A4 | The release commit will be merged to `main` before tagging so `manualUrl`/`/blob/main/` paths resolve | Pitfall 1 | MEDIUM — if the submitted commit isn't on `main`, manualUrl may 404; planner must sequence the merge |

## Open Questions

1. **Merge-to-main strategy for the release commit.**
   - What we know: release work is on `chore/finalise-18hp-docs-and-cleanup` (HEAD `20e47ca`); `main` exists locally + remote.
   - What's unclear: fast-forward vs. merge commit; whether `main` already contains these commits.
   - Recommendation: `git checkout main && git merge --ff-only chore/finalise-...` (or a merge commit if not FF), push `main`, then tag on `main`. Confirm `docs/` present on `main` post-merge. Surface as a discuss-phase decision.

2. **Public email in the submission comment.**
   - What we know: README says include email "if you want it to be public."
   - Recommendation: operator decision — default to omitting it (support routes via GitHub issue). Flag in discuss-phase.

3. **Broader pre-flip secret scan (beyond trial fonts).**
   - What we know: Phase 25 purge scoped to fonts; flip is the last chance to catch any other sensitive blob.
   - Recommendation: run a quick `gh` secret-scan or `git rev-list --all --objects` size/keyword sweep as a belt-and-braces task before flip; not a blocker unless something is found.

4. **`--accept-visibility-change-consequences` flag name/behavior on gh 2.83** (see A2). Verify in a Wave-0 `gh repo edit --help` task before scripting the flip.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `git` | tag/push/mirror-verify | ✓ | 2.39.5 | — |
| `gh` (authed) | flip + issue create | ✓ | 2.83.2, account `Photep` | web UI (manual, unlogged) |
| `git-filter-repo` | (reference only, no re-run) | ✓ | a40bce54 | — |
| `curl` | anon reachability | ✓ | system | anonymous `git clone` |
| GitHub remote reachable | all steps | ✓ | `Photep/ForgeAudio-AnalogSeries` (PRIVATE) | — |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** none — all present.

## Sources

### Primary (HIGH confidence)
- `raw.githubusercontent.com/VCVRack/library/master/README.md` — full verbatim submission process, title=slug rule, commit-hash-not-branch rule, manifest fields, update-via-comment model (fetched 2026-07-10)
- Local repo inspection — visibility (PRIVATE), remote, branch/HEAD (`20e47ca`), tags (`v1.0`–`v1.3`, no `v2.0.0`), clean tree, no Makefile VERSION, plugin.json (`slug`, `version 2.0.0`, `GPL-3.0-or-later`, URLs), gh auth (`Photep`), tool versions
- `.planning/STATE.md` — IP-02 gate CLEARED + "re-confirm at flip time" mandate; purged blob OIDs `031e8db`/`3533f3e`; post-purge `main` `afc1ae2`
- `vcvrack.com/manual/PluginLicensing` — GPLv3+ path, Ethics Guidelines

### Secondary (MEDIUM confidence)
- `github.com/VCVRack/library/issues` — process still issue-based, active submissions July 2026 (NTRWabot, LuxCache, Coalescent)
- WebSearch — v2 builds via `rack-plugin-toolchain`; version now read from `plugin.json`
- `community.vcvrack.com/t/now-taking-submissions-of-v2-plugins-to-the-library/14747` — v2 submission process parity with v1

### Tertiary (LOW confidence)
- gh `--accept-visibility-change-consequences` flag name (A2) — from training knowledge of recent gh; confirm with `gh repo edit --help`

## Metadata

**Confidence breakdown:**
- Submission mechanism (PUB-02): HIGH — verbatim authoritative README + current 2026 issue activity
- Public-flip gate (PUB-01): HIGH — exact commands derived from Phase 25's verified purge (same OIDs/pattern) + git semantics
- Ordering/irreversibility reasoning: HIGH — grounded in the PUB-01 requirement intent
- gh flag exactness (A2) / merge-to-main strategy (A4): MEDIUM — flagged for Wave-0 confirmation

**Research date:** 2026-07-10
**Valid until:** ~2026-08-10 (VCV process is stable; re-check the library README if >30 days elapse)
