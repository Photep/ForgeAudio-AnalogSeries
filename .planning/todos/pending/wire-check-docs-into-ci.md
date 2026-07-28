---
title: Wire tests/check_docs.sh into CI
created: 2026-07-28
source: Phase 29 plan 29-04 (guard-wiring audit, pitfall P-5)
priority: medium
---

`tests/check_docs.sh` is a complete, correct documentation gate written in Phase 27. It passes today. It is invoked by **nothing** — neither the `Makefile` nor `.github/workflows/test.yml` references it, and nothing has since it was written. It has therefore provided exactly zero assurance for its entire existence while appearing, to anyone reading the tests directory, to be an active guard.

What it enforces (all still valid):

- the trademarked-brand-name denylist across `docs/`
- existence of the docs hub and every wave-1 section file
- fact-check tokens (Swing labels, ratio labels, the CV voltage convention) against `src/AnalogLFO.cpp` as the oracle

**The fix is one CI step**, matching the pattern of the three guard steps in the `toolchain-gate` job:

```yaml
      - name: Documentation gate (Phase 27)
        run: bash tests/check_docs.sh
```

Optionally also add `tests/check_docs.sh` to `GUARD_SCRIPTS` in the `Makefile` so `make guards` runs it locally.

**Why it was left out of Phase 29.** Phase 29's subject is the VCO test harness and the LFO non-regression guardrail. `check_docs.sh` is a Phase 27 artifact about the user manual and is unrelated to that guardrail, so wiring it here would have been scope creep into a phase with a published-module risk profile. It was surfaced rather than silently absorbed.

**Current status.** It is the sole entry in `GUARD_WIRING_EXEMPT` in `tests/check_includes.sh` section `[7/7]`, the standing audit that asserts every `tests/check_*.sh` is referenced by the workflow. That audit reports it as `EXEMPT` on every run and points at this file, so the gap stays visible until it is closed. When the CI step lands, remove the exemption entry (section `[7/7]` will then report it as wired) and move this todo to `.planning/todos/done/`.
