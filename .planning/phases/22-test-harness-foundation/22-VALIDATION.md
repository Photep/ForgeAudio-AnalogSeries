---
phase: 22
slug: test-harness-foundation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-14
---

# Phase 22 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest (single-header, drop-in) |
| **Config file** | none — Wave 0 vendors `tests/doctest.h` and appends the `make test` recipe |
| **Quick run command** | `make test` |
| **Full suite command** | `make test` (runs the full doctest binary; multi-rate cases included) |
| **Estimated runtime** | ~{N} seconds (header-only build + run) |

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test`
- **Before `/gsd:verify-work`:** `make test` green AND `make` (plugin build) still succeeds unchanged
- **Max feedback latency:** {N} seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| {N}-01-01 | 01 | 1 | REQ-{XX} | — | N/A | unit | `make test` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

*Populated during planning from RESEARCH.md §"Validation Architecture" — maps TEST-01, TEST-02 (scaffold), TEST-04 and each Success Criterion (±5V bounds, frequency accuracy, phase continuity at reset, fixed-seed determinism, golden-output regression, multi-rate 44.1/48/96 kHz) to a doctest case.*

---

## Wave 0 Requirements

- [ ] `tests/doctest.h` — vendored single-header framework
- [ ] `make test` recipe appended to `Makefile` (additive; does not perturb `make`/`make dist`/`make install`)
- [ ] `tests/` golden-data fixtures captured from current inline DSP (D-08 regression baseline)

*If none: "Existing infrastructure covers all phase requirements."*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| — | — | — | — |

*All phase behaviors have automated verification — the in-Rack listening audition is deferred to Phase 23 (CONTEXT.md `<deferred>`), out of scope here.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < {N}s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
