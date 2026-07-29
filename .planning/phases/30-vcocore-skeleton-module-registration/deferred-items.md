# Phase 30 — Deferred Items

Out-of-scope discoveries logged during execution. Not fixed by the plan that found
them; each names the plan that should resolve it.

---

## 1. `PANEL-03` is marked complete in REQUIREMENTS.md ahead of the work that satisfies it

- **Found during:** plan 30-05, at the `requirements mark-complete` step.
- **Observation:** `requirements.mark-complete PANEL-03 CORE-01` reported both as
  `already_complete` and changed nothing. `git log -S` traces the `- [x] **PANEL-03**`
  line to commit `048d22d` (`docs(30-01)`).
- **Why it is worth a note:** PANEL-03 reads *"The VCO is registered as a second module
  (`addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry) without altering
  the LFO's registration."* None of those three edits has landed — plan **30-06** owns
  all of them, gated on the operator approval recorded in `30-01-SUMMARY.md`. Plan 30-05
  produced the `Model* modelAnalogVCO` symbol that 30-06 will register, and nothing more:
  after 30-05 the module still does not appear in Rack's browser, by design.
- **Not fixed here, deliberately.** The marking predates plan 30-05 and is unrelated to
  any file it touched; un-checking it would be an out-of-scope edit to shared planning
  state mid-phase, and 30-06 lands within the same phase anyway.
- **Resolve at:** plan **30-07**'s phase gate — confirm 30-06 actually landed all three
  registration edits before accepting PANEL-03 as satisfied. If the gate finds them
  missing, PANEL-03 is a false green rather than a bookkeeping quirk.
- **RESOLVED — 2026-07-29, plan 30-07's phase gate. CONFIRMED, not un-checked.**
  All three edits PANEL-03's text names are present and were verified individually:
  `src/plugin.hpp:8` (`extern Model* modelAnalogVCO;`), `src/plugin.cpp:8`
  (`p->addModel(modelAnalogVCO);`) and `plugin.json:26` (`"slug": "ForgeAnalogVCO"`).
  The LFO's registration is unaltered — the manifest parses to exactly 2 modules with
  `ForgeAnalogLFO` still first and `version` still `2.0.1` — and the slug matches
  `src/AnalogVCO.cpp:159`'s `createModel<...>("ForgeAnalogVCO")` character for
  character. The operator's Task 3 sign-off ("Approved") closes the user-visible half:
  the module appears in Rack's browser as `Analog VCO` beside `Analog LFO`.
  **PANEL-03 is genuinely satisfied**; the premature `[x]` at `docs(30-01)` was a
  bookkeeping ordering quirk, not a false green. Full record in `30-07-SUMMARY.md`
  § "Gate findings on the four items earlier plans deferred to this phase gate", item 2.
