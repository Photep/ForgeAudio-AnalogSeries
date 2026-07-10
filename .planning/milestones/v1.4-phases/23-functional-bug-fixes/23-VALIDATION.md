---
phase: 23
slug: functional-bug-fixes
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-14
---

# Phase 23 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest 2.4.11 (vendored `tests/doctest.h`) |
| **Config file** | none — `make test` recipe in `Makefile` (TEST_-namespaced, Rack-free) |
| **Quick run command** | `make test` |
| **Full suite command** | `make test` (single binary, all TUs) |
| **Estimated runtime** | ~5 seconds (confirmed: 35 cases, ~1.6M assertions) |

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test` AND `make RACK_DIR=../Rack-SDK` (plugin must still build)
- **Before `/gsd:verify-work`:** Full suite green + plugin builds + audition decision logged in STATE.md
- **Max feedback latency:** ~10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 23-01 BUG-01 | clock-recovery | A | BUG-01 / TEST-05 | — | re-lock after sustained >3× speedup / fast-clock slowdown band; no permanent lockout | unit (stateful FSM) | `make test` (`test_dsp_stateful.cpp`) | ✅ extend | ⬜ pending |
| 23-02 BUG-03 | swing-display | A | BUG-03 / TEST-05 | — | free-run gates swing warp off (`swingPhaseMultiplier(...,false)==1.0`); effective display swing stored | unit + manual UAT | `make test` (`test_dsp_units.cpp`) | ✅/❌ W0 | ⬜ pending |
| 23-03 BUG-04 | patch-parse | A | BUG-04 / V5 | T-23-01 | malformed seed never throws; falls back to existing seed | unit | `make test` (`test_regression.cpp` + `parseSeedHex`) | ❌ W0 | ⬜ pending |
| 23-04 TEST-03 | unit-coverage | A | TEST-03 | — | waveshape ranges, ratio/alignment table, outlier recovery, swing math covered | unit | `make test` (`test_dsp_units.cpp`) | ⚠️ partial | ⬜ pending |
| 23-05 BUG-02 audition | audition-gate | B | BUG-02 (SC3) | — | keep-current-vs-adopt-table decision logged BEFORE code | **manual / human-verify** | in-Rack listening → STATE.md `### Decisions` | ❌ manual | ⬜ pending |
| 23-06 BUG-02 align | ratio-align | C | BUG-02 / TEST-05 | — | reset cadence per logged decision; no mid-cycle truncation | unit (table) | `make test` (`test_regression.cpp`) | ❌ W0 (gated) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/test_regression.cpp` — NEW TU: BUG-04 `parseSeedHex` pins (Wave A); BUG-02 cadence pins (Wave C, post-audition)
- [ ] `src/dsp/PatchParse.hpp` — NEW: non-throwing `parseSeedHex` (`strtoull`) so BUG-04 earns a headless regression pin
- [ ] `tests/test_dsp_stateful.cpp` — extend: BUG-01 **speedup** re-lock + fast-clock slowdown band cases
- [ ] `tests/test_dsp_units.cpp` — extend TEST-03: waveshape output-range grid, full ratio/alignment-table cases, swing-math edges
- [ ] No framework install needed — doctest + `make test` already in place (Phase 22)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| x1.5/÷1.5 groove desirability (audition) | BUG-02 / SC3 | Subjective musical judgment; gates the code change | Build+install current plugin, flush stale install (hash-compare dylib, relaunch Rack), audition x1.5 & ÷1.5 vs clock, log decision to STATE.md `### Decisions` |
| Phase dot tracks trace (free-run + Medium/Heavy swing) | BUG-03 consumer | GUI render (NanoVG); not reachable from Rack-free harness | Select Medium swing, unplug clock; confirm dot sits on the trace/output, not swing-warped ahead of it |
| `dataFromJson` end-to-end with hand-corrupted patch | BUG-04 wiring | Needs jansson / full module load | Hand-edit a `.vcv` patch seed to `"zzzz"`; load → Rack must not crash; module keeps a valid seed |

---

## Validation Sign-Off

- [ ] All tasks have automated verify or a Wave 0 / manual-only dependency
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify (audition gate is the sole human exception)
- [ ] Wave 0 covers all MISSING references (`parseSeedHex`, `test_regression.cpp`)
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
