---
phase: 26
slug: vcv-library-compliance-packaging
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-09
---

# Phase 26 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest (Rack-free C++ test target) |
| **Config file** | `Makefile` (`TEST_CXXFLAGS`, `test` target) |
| **Quick run command** | `make test` |
| **Full suite command** | `make test` (all doctest TUs) |
| **Estimated runtime** | ~15 seconds |

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test`
- **Before `/gsd:verify-work`:** Full suite must be green locally; CI matrix (ubuntu/macos/windows) green on push
- **Max feedback latency:** ~20 seconds (local); CI latency per push

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 26-01-xx | 01 | 1 | PKG-02, PKG-03 | — / — | N/A | manual+cli | `jq` field asserts + URL reachability | ✅ | ⬜ pending |
| 26-02-xx | 02 | 1 | TEST-06 (D-06) | — / — | N/A | unit | `make test` | ✅ | ⬜ pending |
| 26-03-xx | 03 | 1 | TEST-06 (D-07) | — / — | N/A | unit | `make test` | ✅ | ⬜ pending |
| 26-04-xx | 04 | 2 | PKG-05 | — / — | N/A | manual | `tar --zstd -tvf dist/*.vcvplugin` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky · Task IDs finalized by planner.*

---

## Wave 0 Requirements

*Existing infrastructure covers all phase requirements — the doctest suite, `make test`, and the 3-OS CI matrix already exist (Phase 22 D-09). No framework install needed. New drift-off/spread-off golden fixtures are authored as a Wave-1 task, not a Wave-0 dependency.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| `.vcvplugin` artifact contents | PKG-05 | Requires local `make dist` (real Rack SDK link; not run in CI per D-05) | `make dist` then `tar --zstd -tvf dist/*.vcvplugin` — assert binary, `plugin.json` (populated URLs), `res/`, `LICENSE`; assert NO trial fonts |
| Manifest URLs reachable | PKG-02 | Network fetch of GitHub URLs | `curl -sSfI https://github.com/Photep/ForgeAudio-AnalogSeries` returns 200 |
| CI matrix green on push | TEST-06 | GitHub Actions runs on push (off local machine) | Confirm all three OS legs pass on the pushed commit |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references (none required)
- [x] No watch-mode flags
- [x] Feedback latency < 20s (local)
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-07-09
