---
phase: 25-release-ip-hardening-private
plan: 04
subsystem: ip-compliance
tags: [provenance, svg, ofl, wordmark, fonttools, ip-03, chakra-petch]

requires:
  - phase: 25-release-ip-hardening-private
    provides: 25-02 IP-03 gate (needs-regeneration) that this plan resolves; 25-01 OFL.txt/NOTICES already shipped
provides:
  - All 18 baked-text outlines in res/AnalogLFO.svg re-exported from confirmed-OFL fonts (no trial-FoundationLogo geometry ships)
  - IP-03 closed confirmed-OFL — the precondition 25-03 greps for
affects: [25-03 history-purge, 28-public-flip]

tech-stack:
  added: []
  patterns: [fonttools-SVGPathPen-text-to-outline-re-export, throwaway-venv-no-system-pollution]

key-files:
  created: [.planning/phases/25-release-ip-hardening-private/25-04-SUMMARY.md]
  modified: [res/AnalogLFO.svg]

key-decisions:
  - "PROVENANCE DECISION: confirmed-OFL (face = Chakra Petch, whole panel) — operator-accepted 2026-07-01"
  - "Triage: all 18 baked-text runs share the round-heavy geometric FoundationLogo signature, NOT the shipped monospace JetBrains Mono → all classified trial-derived → all re-exported"
  - "Wordmark generated in both Bebas Neue and Chakra Petch for the gate; operator picked Chakra Petch for one coherent type system across wordmark + functional labels"
  - "Forge-rune (SVG lines 30-34) and all non-text art kept byte-identical; only the 18 text-run lines changed"

patterns-established:
  - "Re-export every baked label from a confirmed-OFL source so provenance is provable by construction, not by eye"

requirements-completed: [IP-03]

duration: ~35min
completed: 2026-07-01
---

# Phase 25 Plan 04: Wordmark + Label OFL Re-export Summary

**All 18 baked-text outlines in res/AnalogLFO.svg were re-exported from confirmed-OFL fonts (Chakra Petch, whole panel) via a throwaway fontTools SVGPathPen pipeline, replacing geometry that almost certainly derived from the trial FoundationLogo. The panel stays path-only (0 font programs), the forge-rune and all non-text art are byte-preserved, and the operator visually accepted the result — PROVENANCE DECISION: confirmed-OFL. IP-03 is closed; 25-03 is unblocked.**

## Performance

- **Duration:** ~35 min
- **Tasks:** 4 (triage-all-18 + functional re-export + wordmark both-faces + human accept/close)
- **Files modified:** 1 (res/AnalogLFO.svg); 18 text-run lines changed, nothing else

## Pipeline (verified live)

Throwaway Python venv (no system pollution, nothing committed) + **fontTools 4.63.0** `SVGPathPen` + **svgpathtools**. OFL fonts fetched to scratch from `github.com/google/fonts/raw/main/ofl/...`, OFL-1.1 headers confirmed for all three. Before/after panels rendered via the Playwright MCP (`file:` blocked → served over `127.0.0.1`, captured through a canvas data-URL since the screenshot tool hit a 5s cap).

Per run: measured the existing outline's cap-height (mm) and horizontal center from its bbox × `s_old`; regenerated the string in the OFL face (UPM 1000, cap 700) with `s_new = cap_mm/700`; held the baseline `Ty` and re-centered via `Tx = center_x − s_new·bbox_mid`; carried the original `fill` (and `opacity` for the footer) verbatim. Emitted bare `<path … transform="translate(Tx,Ty) scale(s,-s)"/>` — the same dialect VCV's nanosvg already renders.

## Triage table — all 18 baked-text runs (classification: ALL trial-FoundationLogo-derived → re-exported)

The 18 runs all share the round-heavy geometric signature of the wordmark (proven ≈FoundationLogo in 25-04-RESEARCH.md), and none match the shipped monospace JetBrains Mono — so every run was classified trial-derived and re-exported. Forge-rune (lines 30-34) = original art, KEPT.

| SVG line | string | role | fill | cap (mm) | center-x (mm) | re-exported from |
|---|---|---|---|---|---|---|
| 28 | `FORGE` | wordmark | #e8612a | 2.723 | 18.50 | Chakra Petch |
| 29 | `AUDIO` | wordmark | #e8612a | 2.723 | 73.18 | Chakra Petch |
| 35 | `ANALOG LFO` | wordmark | #ece8e2 | 3.327 | 45.63 | Chakra Petch |
| 63 | `MORPH` | hero label | #ece8e2 | 2.193 | 45.72 | Chakra Petch |
| 76 | `CHARACTER` | knob label | #c8c4be | 1.437 | 18.03 | Chakra Petch |
| 89 | `DRIFT` | knob label | #c8c4be | 1.386 | 36.32 | Chakra Petch |
| 102 | `RATE` | knob label | #c8c4be | 1.386 | 54.50 | Chakra Petch |
| 115 | `PHASE` | knob label | #c8c4be | 1.437 | 72.74 | Chakra Petch |
| 119 | `MORPH` | jack label | #a39e96 | 1.097 | 7.68 | Chakra Petch |
| 128 | `CHAR` | jack label | #a39e96 | 1.097 | 18.61 | Chakra Petch |
| 137 | `DRIFT` | jack label | #a39e96 | 1.057 | 29.46 | Chakra Petch |
| 146 | `PHASE` | jack label | #a39e96 | 1.097 | 40.31 | Chakra Petch |
| 155 | `FM` | jack label | #a39e96 | 1.057 | 51.20 | Chakra Petch |
| 165 | `CLOCK` | group header | #a39e96 | 1.097 | 67.49 | Chakra Petch |
| 166 | `CLK` | clock jack | #c8c4be | 1.134 | 62.07 | Chakra Petch |
| 169 | `RST` | clock jack | #c8c4be | 1.134 | 72.93 | Chakra Petch |
| 172 | `OUT` | clock jack (ember) | #ff8a4c | 1.286 | 83.77 | Chakra Petch |
| 177 | `forgeaudio` | footer | #e8612a @0.4 | 1.259 | 45.71 | Chakra Petch |

## OFL source fonts (provenance of record)

| Font | Used for | Source | OFL |
|---|---|---|---|
| **Chakra Petch** (Regular) | all 18 runs (chosen face) | `github.com/google/fonts/raw/main/ofl/chakrapetch/ChakraPetch-Regular.ttf` — Version 1.000 | SIL OFL 1.1 ✓ |
| **Bebas Neue** (Regular) | wordmark variant generated for the gate (not shipped) | `github.com/google/fonts/raw/main/ofl/bebasneue/BebasNeue-Regular.ttf` — Version 2.000 | SIL OFL 1.1 ✓ |
| JetBrains Mono | candidate for monospace labels (not needed — labels are proportional) | `…/ofl/jetbrainsmono/JetBrainsMono[wght].ttf` — Version 2.211 | SIL OFL 1.1 ✓ |

No `.ttf` is committed — only static outlines ship (res/fonts/OFL.txt, shipped in 25-01, already carries the SIL OFL 1.1 text).

## Verification

- Census across res/AnalogLFO.svg **and** res/components/*.svg = **0** font/text/glyph/@font-face/base64/use nodes (still path-only)
- `xml.dom.minidom` parse OK (well-formed); viewBox `0 0 91.44 128.5` unchanged; 115 `<path>` retained
- `git diff res/AnalogLFO.svg` = exactly **18 lines** changed (the 18 text runs); forge-rune lines 30-34 byte-identical; all knob/jack/display/rail/bolt/gradient art unchanged
- Both candidate panels rendered (Chakra + Bebas) and compared against the original before render; placement/fill preserved; wordmark character intentionally differs (no OFL face reproduces the round-heavy FoundationLogo — provenance is the gate, not pixel parity)

## Provenance decision (verbatim)

**`confirmed-OFL: chakra`** — operator visually accepted the Chakra Petch panel on 2026-07-01. Every shipped text outline now provably derives from a confirmed-OFL font; no trial-FoundationLogo geometry ships anywhere in the panel.

## Self-Check: PASSED

IP-03 closed confirmed-OFL. 25-03's precondition (this SUMMARY contains the literal `confirmed-OFL`) is satisfied — the irreversible history purge may now proceed.

## Downstream

25-03 may run: 25-01's tree changes + this regenerated panel must be committed locally, then pushed (push-first) before the filter-repo purge. The corrected, OFL-clean working tree is what the purge will preserve.
