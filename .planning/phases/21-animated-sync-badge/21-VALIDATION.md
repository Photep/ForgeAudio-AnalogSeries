---
phase: 21
slug: animated-sync-badge
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-13
---

# Phase 21 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
>
> **Medium note:** This VCV Rack C++ plugin has **no automated test suite** (confirmed in
> RESEARCH.md §Validation Architecture and project build memory). The compile gate
> (`make RACK_DIR=../Rack-SDK`) is the only automated signal; all behavioral verification is
> manual in-Rack visual UAT. Phase 21 is a 1-file render animation with two purely visual
> success criteria, so this is expected, not a gap.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None — no unit/integration harness exists for this VCV plugin |
| **Config file** | none |
| **Quick run command** | `make RACK_DIR=../Rack-SDK` (compile gate — the only automated signal) |
| **Full suite command** | `make RACK_DIR=../Rack-SDK && make install RACK_DIR=../Rack-SDK` then manual in-Rack UAT |
| **Estimated runtime** | ~10–30 seconds (compile); UAT manual |

---

## Sampling Rate

- **After every task commit:** Run `make RACK_DIR=../Rack-SDK` — must compile clean.
- **After every plan wave:** `make RACK_DIR=../Rack-SDK && make install RACK_DIR=../Rack-SDK`, then sync the extracted plugin dir + relaunch Rack and spot-check the badge is unchanged at rest.
- **Before `/gsd:verify-work`:** Compile clean + full visual UAT (all four checks below) green.
- **Max feedback latency:** ~30 seconds (compile); visual UAT is the human acceptance gate.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 21-01-* | 01 | 1 | ANIM-01 | — | N/A (no security surface — in-process render) | manual (visual) + compile gate | `make RACK_DIR=../Rack-SDK` | ✅ (src/AnalogLFO.cpp exists) | ⬜ pending |
| 21-01-* | 01 | 1 | ANIM-02 | — | N/A | manual (visual) + compile gate | `make RACK_DIR=../Rack-SDK` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*(Exact task IDs resolve once PLAN.md is written; both phase requirements map to the single SYNC-badge animation plan.)*

---

## Wave 0 Requirements

*None — no test infrastructure to create. The project is intentionally compile-gate + manual UAT (RESEARCH.md §Validation Architecture: "no test infrastructure to create"). Each implementation task must end with a clean compile; the phase ends with a human visual acceptance gate where the locked tuning constants (peak white-point, glow blur, glow alpha) are decided.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| SYNC badge flashes on each LOCKED clock edge, visibly distinct from steady ember | ANIM-01 | Visual emissive-render behavior; no test harness can assert NanoVG pixel output | Patch a clock source into CLK; once LOCKED, confirm a bright white-hot pulse on the badge on each beat, clearly brighter than steady state (color shift + glow bloom, not alpha) |
| Flash decays smoothly (~200ms feel) with no abrupt transition / strobe | ANIM-02 | Smoothness is a perceptual judgment | Watch the badge over several beats and via screen capture; confirm a smooth exponential cool-down to steady ember, no hard cut-off or flicker |
| ACQUIRING still blinks at 2 Hz with NO per-edge flash (D-02 separation) | ANIM-01 / D-02 | Regression check on a mutually-exclusive animation state | Before lock, confirm the 2 Hz blink behaves exactly as before and no per-edge flash fires during acquisition |
| No flash when free-running / unclocked (badge faded out) | D-02 | State-gated visual behavior | Remove the clock; confirm badge fades out and never flashes |

---

## Validation Sign-Off

- [ ] All tasks end with a clean compile (`make RACK_DIR=../Rack-SDK`); no automated unit verify possible (no harness)
- [ ] Sampling continuity: compile gate runs after every task commit (manual UAT covers behavior)
- [ ] Wave 0 covers all MISSING references — N/A (no infrastructure to create)
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s (compile)
- [ ] Manual UAT covers all four behaviors above before `/gsd:verify-work`
- [ ] `nyquist_compliant: true` set in frontmatter (after sign-off — manual-only is the documented, accepted strategy for this medium)

**Approval:** pending
