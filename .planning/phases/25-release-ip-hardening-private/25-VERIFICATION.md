---
phase: 25-release-ip-hardening-private
verified: 2026-07-08T00:00:00Z
status: passed
score: 5/5 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: none
  previous_score: none
---

# Phase 25: Release IP Hardening (PRIVATE) Verification Report

**Phase Goal:** The repository is legally clean for public GPL release — trial fonts purged from all git history while still private and verified gone, with LICENSE, NOTICES, and confirmed asset provenance.
**Verified:** 2026-07-08
**Status:** passed
**Re-verification:** No — initial verification
**Method:** Read-only verification against the LIVE working tree + LOCAL git mirror. No network/remote operations and no git write/rewrite/push commands were run (this phase already completed an irreversible remote purge). Remote-push + clean-room re-clone for IP-02 are attested by 25-03-SUMMARY.md and corroborated here by confirming the local mirror is clean.

## Goal Achievement

### Observable Truths (ROADMAP Success Criteria)

| # | Truth (SC) | Req | Status | Evidence |
|---|-----------|-----|--------|----------|
| 1 | `BCBarellTEST-Regular.otf` + `FoundationLogo.ttf` removed from working tree AND gitignored | IP-01 | ✓ VERIFIED | `ls` → both "No such file or directory"; `git check-ignore` exits 0 for both; `.gitignore` lines 20–21 list both names |
| 2 | Both trial fonts purged from ALL git history (filter-repo, force-pushed to still-private remote `--all --tags`) | IP-02 | ✓ VERIFIED | Local mirror clean: `git cat-file -e 031e8db9…` (BCBarell) → MISSING (exit 1), `…3533f3e4…` (FoundationLogo) → MISSING (exit 1). Remote force-push + private-repo status attested in 25-03-SUMMARY (main `afc1ae2`, tag v1.3 `1f7441e`, `gh repo view isPrivate=true` before+after). Not re-run (network/write forbidden by scope). |
| 3 | Purge VERIFIED clean via fresh clone: `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` empty (hard gate for Phase 28) | IP-02 | ✓ VERIFIED | Local `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` → EMPTY (grep exit 1). `git ls-tree -r v1.3 \| grep …` → EMPTY. Authoritative clean-room second-clone verification recorded in 25-03-SUMMARY; this is the corroborating local-mirror re-confirmation. |
| 4 | GPL-3.0 `LICENSE` at repo root + Makefile `DISTRIBUTABLES` picks it up | PKG-01 | ✓ VERIFIED | `LICENSE` present, 674 lines, header "GNU GENERAL PUBLIC LICENSE / Version 3, 29 June 2007". `Makefile:14` → `DISTRIBUTABLES += $(wildcard LICENSE*)` |
| 5 | NOTICES inventories every shipped third-party asset license (JetBrains Mono OFL, panel SVG outlines) + font-outline provenance confirmed acceptable for GPL release | PKG-04, IP-03 | ✓ VERIFIED | `NOTICES` present, names plugin GPL-3.0-or-later, JetBrains Mono OFL-1.1, Bebas Neue + Chakra Petch OFL-1.1; `res/fonts/OFL.txt` present (SIL OFL 1.1 verbatim). `res/AnalogLFO.svg` census = 0 text/glyph/font/@font-face/use/base64 nodes (path-only). 25-04-SUMMARY records provenance decision `confirmed-OFL` (Chakra Petch) with recorded OFL font sources. `Makefile:15` → `DISTRIBUTABLES += $(wildcard NOTICES*)` |

**Score:** 5/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `LICENSE` | Verbatim GPL-3.0 at repo root | ✓ VERIFIED | 674 lines, correct FSF header |
| `NOTICES` | Third-party inventory | ✓ VERIFIED | 3 sections: plugin GPL, JetBrains Mono OFL, wordmark outlines Bebas Neue/Chakra Petch OFL |
| `res/fonts/OFL.txt` | Verbatim SIL OFL 1.1 | ✓ VERIFIED | JetBrains Mono OFL.txt header + OFL 1.1 body |
| `.gitignore` | Both trial fonts listed | ✓ VERIFIED | Lines 20–21 |
| `Makefile` | DISTRIBUTABLES ships LICENSE + NOTICES | ✓ VERIFIED | Lines 13–16 (res, LICENSE*, NOTICES*, presets) |
| `res/AnalogLFO.svg` | Path-only, OFL-derived outlines | ✓ VERIFIED | 0 font programs; 120 `<path>` nodes |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| Makefile | LICENSE | `$(wildcard LICENSE*)` glob | ✓ WIRED | Line 14 |
| Makefile | NOTICES | `$(wildcard NOTICES*)` glob | ✓ WIRED | Line 15 |
| Makefile | res/fonts/OFL.txt | `DISTRIBUTABLES += res` glob | ✓ WIRED | Line 13 (OFL.txt lives under res/) |
| IP-03 gate | 25-03 purge precondition | `confirmed-OFL` literal in 25-04-SUMMARY | ✓ WIRED | Decision recorded; purge correctly proceeded only after confirmed-OFL |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Regression suite green (docs/asset-only changes) | `make test` | 47/47 passed, 2,590,445 assertions, exit 0 | ✓ PASS |
| Trial-font blobs unreachable locally | `git cat-file -e <both OIDs>` | both MISSING (exit 1) | ✓ PASS |
| No trial-font objects in any history | `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` | EMPTY | ✓ PASS |
| v1.3 tag tree font-free | `git ls-tree -r v1.3 \| grep …` | EMPTY | ✓ PASS |
| plugin.json untouched this phase | `git log --oneline -- plugin.json` | last change `a83dcb0` (Phase 19); working tree clean | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Status | Evidence |
|-------------|-------------|--------|----------|
| IP-01 | 25-01 | ✓ SATISFIED | Fonts absent + gitignored (SC1) |
| IP-02 | 25-03 | ✓ SATISFIED | Local mirror clean; remote purge + clean-room re-clone attested in 25-03-SUMMARY (SC2, SC3) |
| IP-03 | 25-02 → 25-04 | ✓ SATISFIED | Gate returned needs-regeneration; wordmark re-exported from Chakra Petch OFL; closed confirmed-OFL (SC5) |
| PKG-01 | 25-01 | ✓ SATISFIED | GPL-3.0 LICENSE + Makefile glob (SC4) |
| PKG-04 | 25-01 | ✓ SATISFIED | NOTICES + OFL.txt shipped + wired (SC5) |

### Anti-Patterns Found

None. No `TBD`/`FIXME`/`XXX` debt markers in any phase-modified file (LICENSE, NOTICES, OFL.txt, .gitignore, Makefile, res/AnalogLFO.svg).

### Notes / Observations (non-blocking)

- **Hash drift vs. 25-03-SUMMARY:** the summary recorded main `afc1ae2` and tag `v1.3=1f7441e` at purge completion; the local mirror now reads HEAD `185e68b` and `v1.3=f08577e`. History has advanced past the recorded snapshot (later planning commits). This does NOT affect cleanliness — all three IP-02 grep/cat-file checks return clean against the current local state, so whatever the current commits/tag point at is font-free. Informational only.
- **Remote/clean-room re-clone not independently re-run:** per scope, no network or git write operations were performed. IP-02's remote force-push and second independent clean-room clone are attested by 25-03-SUMMARY (with commit hashes and `gh isPrivate=true` before+after). The local-mirror confirmation here is strong corroborating evidence that the purge landed. Phase 28's public-flip gate (PUB-01) should re-confirm EMPTY on a final fresh clone at flip time — this is already the planned downstream gate, not a Phase 25 gap.
- **PKG-02/PKG-03 correctly deferred to Phase 26:** plugin.json was NOT modified in this phase (last touched Phase 19 `a83dcb0`); license `GPL-3.0-or-later` and version `2.0.0` already correct.

### Gaps Summary

None. All five ROADMAP success criteria are observably true in the live tree and local git mirror. Trial fonts are absent + gitignored, purged from all reachable local history (both blob OIDs unreachable, rev-list + v1.3 tree grep EMPTY), the GPL-3.0 LICENSE + NOTICES + OFL.txt are present and wired into the dist via Makefile globs, the panel SVG is path-only with confirmed-OFL (Chakra Petch) provenance, and the 47/47 test suite stays green. The phase goal — a legally clean repository for public GPL release — is achieved.

---

_Verified: 2026-07-08_
_Verifier: Claude (gsd-verifier)_
