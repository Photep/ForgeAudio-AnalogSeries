---
phase: 25-release-ip-hardening-private
plan: 02
subsystem: ip-compliance
tags: [provenance, svg, ofl, wordmark, ip-03, human-gate]

requires:
  - phase: 25-release-ip-hardening-private
    provides: 25-01 legal-clean tree (LICENSE/NOTICES/OFL) the provenance review supplements
provides:
  - Automated census proving zero font programs ship in any res SVG
  - Recorded human provenance decision for IP-03 / Assumption A1
affects: [25-03 history-purge, 28-public-flip, wordmark-regeneration-followup]

tech-stack:
  added: []
  patterns: [human-gate-records-decision-for-auditable-phase-28-flip]

key-files:
  created: [.planning/phases/25-release-ip-hardening-private/25-02-SUMMARY.md]
  modified: []

key-decisions:
  - "PROVENANCE DECISION: needs-regeneration — operator could NOT positively confirm the wordmark outlines derive from OFL fonts"
  - "Design-language memory documents FoundationLogo (the trial font) as the brand face, raising the risk the shipped outlines were drawn from the trial face"
  - "res/AnalogLFO.svg carries zero font metadata, so provenance is undeterminable from the file — human judgment required and exercised"

patterns-established:
  - "A blocking human gate can return a phase-halting verdict; 25-03 must NOT proceed on needs-regeneration"

requirements-completed: [IP-03]

duration: ~5min
completed: 2026-07-01
---

# Phase 25 Plan 02: Human Provenance Gate Summary

**The shipped panel SVGs embed no font program (automated census, 0 font/glyph nodes across all 10 files), but the operator could NOT positively confirm the FORGE/AUDIO + "ANALOG LFO" wordmark outlines derive from OFL fonts — PROVENANCE DECISION: needs-regeneration. This BLOCKS the irreversible 25-03 history purge until the wordmark is re-exported from a confirmed-OFL font.**

## Performance

- **Duration:** ~5 min
- **Tasks:** 2 (1 automated census + 1 blocking human gate)
- **Files modified:** 0 (read-only review, as designed)

## Accomplishments

### Task 1 — Automated SVG font-data census
Census across `res/AnalogLFO.svg` plus all sibling panel SVGs (`res/components/*.svg`):

| File | font/text/glyph/@font-face/base64 | `<use>` (A3) | `<path>` |
|------|-----------------------------------|--------------|----------|
| res/AnalogLFO.svg | 0 | 0 | 115 |
| res/components/*.svg (9 files) | 0 each | 0 each | 0 each |
| **Aggregate** | **0** | **0** | — |

- ✓ No font program ships in any SVG (no `<text>`, `<tspan>`, `<glyph>`, `<font>`, `@font-face`, or base64-embedded font binary).
- ✓ A3 ruled out — no `<use href>` pulling a glyph symbol.
- ✓ The wordmark is static Bézier path geometry only.
- ⚠️ The SVG carries **no** `font-family` attribute, layer label, or metadata naming the source font — provenance is undeterminable from the file.

### Task 2 — Human provenance gate (IP-03 / Assumption A1) — BLOCKING

**Evidence surfaced to the operator:** the automated census above, plus the flag that the
project design-language memory explicitly documents *"FoundationLogo font for brand"* — i.e.
the **trial** font (being purged) is recorded as the intended brand face, so the shipped
outlines may have been drawn from the trial font.

**OPERATOR DECISION (verbatim): `needs-regeneration`**

The operator could not positively confirm that the FORGE/AUDIO and "ANALOG LFO" wordmark
outlines were drawn from the OFL fonts (Bebas Neue / Chakra Petch) rather than the trial
FoundationLogo. Per the plan's `resume-signal` contract, this means the wordmark paths must
be re-exported from a confirmed-OFL font before the history purge.

## Consequence — Phase 25 is HALTED

Per 25-03's stated PRECONDITION ("25-02 must have returned 'confirmed-OFL'; if
'needs-regeneration', STOP — a wordmark re-export plan must land and be committed first"),
**plan 25-03 will NOT run.** The irreversible remote history rewrite is correctly blocked.

### Required follow-up before 25-03 can proceed
1. Add a new plan (e.g. 25-04 or a Phase-25 amendment) to **re-export the FORGE/AUDIO +
   "ANALOG LFO" wordmark outlines in `res/AnalogLFO.svg` from a confirmed-OFL font**
   (Bebas Neue and/or Chakra Petch), replacing any geometry that may have derived from the
   trial FoundationLogo.
2. Land + commit that regeneration on local `main`.
3. Re-run the IP-03 provenance gate and obtain `confirmed-OFL`.
4. Only then execute 25-03 (push-first → purge → force-push → clean-room verify).

## Self-Check: PASSED

The plan's job was to **record a provenance decision** (confirmed-OFL or needs-regeneration);
that decision is recorded here and mirrored into STATE.md. The blocking verdict is the correct,
auditable result — not a failure of this plan.
