# Phase 29: VCO Test Harness & LFO Non-Regression Guardrail - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-27
**Phase:** 29-vco-test-harness-lfo-non-regression-guardrail
**Areas discussed:** VcoCore seam vs. skeleton, Guardrail paranoia level, VCO strict/MinGW proof, Day-one harness invariants

---

## VcoCore seam vs. skeleton

Central tension: roadmap says the harness lands "before any VCO DSP," yet criterion 2 needs it to "drive a `forge::VcoCore`" — and Phase 30 is titled "VcoCore Skeleton."

| Option | Description | Selected |
|--------|-------------|----------|
| Bare POD seam only | `VcoCore.hpp` = POD boundary; `step()` returns silence. All DSP in Phase 30. | ✓ |
| Minimal naive oscillator | Real-but-trivial body (phase accumulator + naive sine) so invariants test real behavior now. Mirrors Phase 22 D-03. | |
| Pull full skeleton forward | Absorb Phase 30's whole skeleton into 29; Phase 30 shrinks to registration. | |

**User's choice:** Bare POD seam only.
**Notes:** Deliberately the opposite of Phase 22's D-03 ("pull real behavior forward"). Keeps the 29/30 boundary crisp; the meaningful behavioral canary for Phase 29 is the shipped-LFO golden replay, not the nonexistent VCO DSP.

### Sub-question — POD Inputs completeness

| Option | Description | Selected |
|--------|-------------|----------|
| Full input surface now | Declare complete VCO input contract up front; later phases fill behavior only. | |
| Minimal, grow per phase | Declare only what plumbing needs now; add fields per owning phase. | |
| You decide | Planner derives field set from REQUIREMENTS + LfoCore precedent. | ✓ |

**User's choice:** You decide (seam *shape* fixed; field set = planner discretion).

---

## Guardrail paranoia level

Existing byte-exact/1e-6 golden replay catches behavioral drift only if output changes AND `make test` re-runs. Milestone language: "no later phase can *silently* threaten the live LFO."

| Option | Description | Selected |
|--------|-------------|----------|
| Golden-file checksum lock | SHA-256 of each LFO `.f32`; fails if bytes change. Catches "regenerate goldens to pass." | ✓ |
| Frozen-header hash guard | SHA-256 manifest of the 4 frozen headers; fails if edited without a manifest bump. Phase 34 bumps deliberately. | ✓ |
| Shared-header include audit | CI grep guarding dependency direction (VCO code calls, never edits, frozen headers; no LFO TU includes VCO files). | ✓ |
| Replay alone is enough | Trust existing replay; add nothing. | |

**User's choice:** All three tripwires (belt-and-suspenders).
**Notes:** Recorded as hard-fail CI gates. Frozen-header guard covers `DriftEngine.hpp`; Phase 34's additive edit uses a sanctioned one-line manifest bump — which forces that edit to surface (a feature).

---

## VCO strict/MinGW proof

Gap: both gates only compile headers reached via a `.cpp`; nothing in `src/` includes `VcoCore.hpp` until Phase 30, so criterion 3 would be wired but not proven in P29.

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated compile canary | Compile-only unit including every VCO header, forced through both `-pedantic-errors` and the MinGW link-vs-`libRack` leg. Grows as headers are born. | ✓ |
| Minimal AnalogVCO.cpp stub | Non-registered `src/AnalogVCO.cpp` including `VcoCore.hpp`; joins `src/*.cpp` glob. | |
| Glob-and-verify-at-P30 | Rely on glob; verify coverage in P30. | |

**User's choice:** Dedicated compile canary.
**Notes:** Permanent + growing (adds `MorphBlep.hpp` in P32, etc.). Must exercise BOTH the C++11 syntax gate and the MinGW compile+link leg — the link leg is the only thing that catches the in-class `static constexpr` ODR class that rejected v2.0.0.

---

## Day-one harness invariants

| Option | Description | Selected |
|--------|-------------|----------|
| Structural plumbing set | Rack-free at 3 rates, sampleTime injection, non-degenerate seeds, run-to-run determinism, finite/no-NaN. Semantic asserts deferred. | |
| Plumbing + reserved skeletons | Above plus empty/skipped skeletons for deferred semantic invariants. | |
| You decide | Planner chooses from BlockDriver precedent. | ✓ |

**User's choice:** You decide.
**Notes:** Recommended default recorded in CONTEXT.md = the structural plumbing set (right-sized for a stub); semantic asserts (< 1-cent P31, alias-floor P32, ±5V bounds P34) deferred to owning phases.

---

## Claude's Discretion

- POD `Inputs` field set — derive from REQUIREMENTS + `LfoCore` `Inputs`; recommendation leans toward near-term (P30/P31) fields without over-speculating, seam shape fixed.
- Day-one `VcoBlockDriver` invariant set — recommended structural plumbing set; reserved skeletons optional.
- Guard mechanism/location (doctest cases vs. CI shell steps), hash-manifest format/location, canary-TU location.

## Deferred Ideas

None — all VCO-behavioral topics were correctly routed to their owning phases (P30/P31/P32/P34/P36) rather than pulled into this guardrail phase.
