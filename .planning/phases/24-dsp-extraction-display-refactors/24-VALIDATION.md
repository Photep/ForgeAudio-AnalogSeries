---
phase: 24
slug: dsp-extraction-display-refactors
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-26
---

# Phase 24 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest (single-header `tests/doctest.h`) + `forge::BlockDriver` |
| **Config file** | none — `Makefile` `test` target, Rack-free, `-ffp-contract=off` |
| **Quick run command** | `make test` |
| **Full suite command** | `make test` |
| **Estimated runtime** | ~sub-second |

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test` + `make` (plugin still builds against `../Rack-SDK`)
- **Before `/gsd:verify-work`:** Full suite must be green AND manual in-Rack UAT logged to STATE.md
- **Max feedback latency:** ~5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 24-W0 | 00 | 0 | CLEAN-05 | — | N/A | unit | `make test` | ❌ W0 (`tests/test_display.cpp`) | ⬜ pending |
| 24-W0 | 00 | 0 | CLEAN-04 | — | N/A | unit | `make test` | ❌ W0 (`tests/test_anim.cpp`) | ⬜ pending |
| CLEAN-05 | — | — | CLEAN-05 | — | Fill is a pure, thread-independent fn of the snapshot (byte-identical) | unit | `make test` | ❌ W0 | ⬜ pending |
| CLEAN-05 | — | — | CLEAN-05 | — | Swing remap correct at the 0.5001 boundary | unit | `make test` | ❌ W0 | ⬜ pending |
| CLEAN-04 | — | — | CLEAN-04 | — | Pathological/NAN/neg dt clamps to one cap step (→0 on non-finite) | unit | `make test` | ❌ W0 | ⬜ pending |
| CLEAN-04 | — | — | CLEAN-04 | — | Decay feel-identical at 60fps (`pow(0.92,dt*60)`) | unit | `make test` | ❌ W0 | ⬜ pending |
| CLEAN-03 | — | — | CLEAN-03 | — | Ratio/BPM pill fade symmetry on clock disconnect | manual UAT | n/a (visual, human-gated per D-06) | — | ⬜ pending |
| CLEAN-01 | — | — | CLEAN-01 | — | Dead code (`drawZeroCrossing`, `scanlineImage`) removed; builds clean | compile | `make` | n/a | ⬜ pending |
| CLEAN-02 | — | — | CLEAN-02 | — | Unreachable `isStill` resolved; builds clean | compile | `make` | n/a | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/test_display.cpp` — covers CLEAN-05 (fill purity + swing remap). Depends on `src/dsp/DisplayFill.hpp`.
- [ ] `tests/test_anim.cpp` — covers CLEAN-04 (clamp + decay). Depends on `src/dsp/Anim.hpp`.
- [ ] Framework install: none (doctest vendored; new TUs auto-globbed by `TEST_SOURCES := $(wildcard tests/*.cpp)`).

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ratio/BPM pill fade symmetry | CLEAN-03 | Inherently visual; no headless render | In-Rack: disconnect clock, observe ratio/BPM pills fade out over 200ms in sync with the SYNC badge (no instant pop) |
| Overall animation/fade feel | CLEAN-04 | Subjective visual feel; `getLastFrameDuration()` semantics flagged MEDIUM | In-Rack: audition badge decay, breathe, blink, scanline scroll, fades — must feel identical to pre-refactor at 60fps; no stutter after tab-out |
| Display unchanged after dead-code removal | CLEAN-01/02 | Visual regression check | In-Rack: confirm waveform display renders identically |

*Per D-06: "test what's deterministic, human-gate the visual."*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references (`tests/test_display.cpp`, `tests/test_anim.cpp`)
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
