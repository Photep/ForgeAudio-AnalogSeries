---
phase: 27
slug: notion-manual
status: verified
threats_open: 0
asvs_level: 1
created: 2026-07-10
---

# Phase 27 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Screen capture → committed PNG | A screenshot could inadvertently include other on-screen windows, file paths, or PII | Panel screenshot image (`docs/img/panel-raw.png`, `panel-annotated.png`) |
| Repo → public reader (Phase 28) | Committed docs + images become publicly readable at the Phase 28 public flip | Public manual prose, images, manifest URLs |
| Manifest link → external target | `plugin.json.manualUrl` points users at a GitHub URL | Link target (owner/repo/branch) |
| Bundled third-party assets → redistribution | Fonts / glyph outlines shipped in the plugin | Font files + outlined glyph geometry |

---

## Threat Register

| Threat ID | Category | Component | Disposition | Mitigation | Status |
|-----------|----------|-----------|-------------|------------|--------|
| T-27-01 | Information disclosure | docs/*.md prose | mitigate | Public control/behavior facts only; grep confirms no secrets/tokens/absolute paths/PII | closed |
| T-27-02 | Tampering (legal/IP) | manual prose (engine-concept, panel legend) | mitigate | Generic vocabulary; `tests/check_docs.sh` brand-name denylist returns zero hits | closed |
| T-27-03 | Repudiation (attribution) | license-credits.md | mitigate | Links authoritative repo-root `LICENSE` / `NOTICES` / `res/fonts/OFL.txt`; declares GPL-3.0-or-later matching `plugin.json` | closed |
| T-27-04 | Information disclosure | changelog / install prose | mitigate | Public feature/install facts only; grep confirms no internal paths/secrets/PII | closed |
| T-27-05 | Tampering (manifest integrity) | plugin.json | mitigate | `json.load` valid; version `2.0.0` unchanged; prior URLs retained | closed |
| T-27-06 | Spoofing (link target) | plugin.json manualUrl | mitigate | manualUrl repo `Photep/ForgeAudio-AnalogSeries` matches verified sourceUrl repo | closed |
| T-27-07 | Information disclosure | manualUrl reachability | accept | Link 404s publicly until the Phase 28 repo-public flip — by design (sequencing) | closed |
| T-27-08 | Information disclosure | docs/img/panel-raw.png, panel-annotated.png | mitigate | Capture cropped tightly to the module panel; human-verified + independently re-inspected — no other windows/paths/PII in frame | closed |
| T-27-09 | Tampering (build integrity) | screenshot source build | mitigate | Built vs installed `plugin.dylib` shasum matched (`a9c4731…`) — annotated panel reflects the shipped build | closed |
| T-27-SC | Tampering (supply chain) | npm/pip/cargo installs | accept | No dependency-manifest changes this phase; build uses existing `../Rack-SDK`, Playwright already present | closed |

*Status: open · closed*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-27-01 | T-27-07 | Manual URL is unreachable to the public until the Phase 28 repo-public flip; this is the intended publish sequence (DOC-03 closes in Phase 28) | chris | 2026-07-10 |
| AR-27-02 | T-27-SC | No package installs occur in a docs/screenshot phase; tooling (Rack SDK, Playwright) pre-exists | chris | 2026-07-10 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-07-10 | 10 | 10 | 0 | secure-phase (evidence-verified) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-07-10
