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

---

## 2. Every live VCO instance in a patch is a bit-identical clone (WR-02)

- **Found during:** Phase 30 code review and verification; carried into gap closure by
  the operator's UAT test 3 answer, option (a).
- **Observation:** the constructor at `src/AnalogVCO.cpp:96-97` hardcodes both the drift
  seed and the spread seed, so `setSpreadSeed` — which is the WHOLE of this phase's
  per-instance divergence mechanism (D-11) — receives the same pair in every instance.
  Measured at **0 of 2048 differing samples** between two identically-constructed
  modules, reproduced independently by the reviewer (`30-REVIEW.md`) and by the verifier
  (`30-UAT.md`).
- **Why it is worth a note:** roadmap success criterion 4 ("same seed → bit-identical
  block; different seed diverges") is satisfied at the `forge::VcoCore` level and proven
  non-vacuously by two passing invariants — but those invariants drive two
  **differently-seeded** cores, which the shell never constructs. A reader could take
  the test evidence as describing the shipped module. Plan 30-09 corrected the
  constructor comment so the source no longer makes that claim; the behavior itself is
  still open.
- **Not fixed here, deliberately.** No requirement in `REQUIREMENTS.md` asks for
  per-instance shell entropy in Phase 30; the hardcoded seed was a planned 30-05
  must-have (T-30-02) chosen to avoid a real Rack hang on a degenerate `(0,0)` pair; and
  the operator scoped only the comment correction into this gap closure.
- **Resolve at:** **Phase 34/35**, when the shell gains its analog-engine wiring and
  patch persistence. The pattern already exists and does not need designing — the
  shipped Analog LFO module draws its seed from `std::random_device`, rejects a `(0,0)`
  pair, persists the drawn spread seed in the patch, and restores it through a
  non-throwing hex parser. **Any implementation MUST re-validate a deserialized value
  the same way:** a corrupt or zero pair restored from a patch file is the same hang
  (a `(0,0)` Xoroshiro pair is a fixed point emitting an all-zero stream, so the
  rejection loop inside `std::normal_distribution` never terminates).

---

## 3. `forge::clamp` is NaN-transparent, so VcoCore's defensive clamps are inert (CR-02)

- **Found during:** Phase 30 code review; accepted by the operator at UAT test 2.
- **Observation:** `forge::clamp` is a comparison ladder, so both comparisons are false
  for NaN and the value passes through unchanged. `src/dsp/VcoCore.hpp`'s morph and
  character clamps are the core's only defensive validation, and they are inert against
  exactly the input class a defensive clamp exists to stop. This is a silent divergence
  from `rack::math::clamp`, which is built from `fmin`/`fmax` and discards a NaN operand.
- **Why it is worth a note:** unreachable today, because Rack sanitises NaN in
  `ParamQuantity::setValue` before a param is read — and reachable the moment
  MORPH/CHARACTER **CV inputs** land, because Rack does not sanitise cable voltages and a
  non-finite sample handed to the output propagates through the user's whole patch.
- **Not fixed here, deliberately.** The operator accepted it for Phase 30.
- **Resolve at:** **Phase 31 or Phase 34**, in the same plan that adds the MORPH/CHARACTER
  CV inputs. **CONSTRAINT ON ANY FIX:** `forge::clamp` lives in a header that is
  byte-pinned by `tests/check_frozen.sh` and is consumed by the **shipped** Analog LFO at
  `src/dsp/LfoCore.hpp:168,212-213,216`. Editing that shared primitive is a guardrail
  event, not a VCO fix. The fix is a NaN-safe helper **local to `VcoCore`**, bit-identical
  to the shared primitive for finite inputs, pinned by a case that fails before it lands.

---

## 4. `plugin.json` still declares version 2.0.1 while shipping a second module (WR-04)

- **Found during:** Phase 30 code review.
- **Observation:** `v2.0.1` is tagged and live in the VCV Library with a single module;
  the manifest now declares two modules under that same version string.
- **Why it is worth a note:** the library builds from git tags, so two artifacts carrying
  one version string describe different plugins.
- **Not fixed here, deliberately.** This is D-04, an explicit Phase 30 decision to hold
  the version — recorded here so a stale version is distinguishable from a forgotten one.
- **Resolve at:** **Phase 36** (REL-01), which owns the tag, the changelog entry and the
  #929 update comment, and which must re-observe the CI link leg on whatever commit it
  tags.

---

## 5. `tests/check_includes.sh [2/7]`'s exemption filter is unanchored (WR-05), plus the remaining Info findings

- **Found during:** Phase 30 code review.
- **Observation:** the `[2/7]` exemption is applied as a line-level exclusion with no
  anchors, so any output line containing the exempted text *anywhere* is discarded —
  including a line whose actual directive is a real SDK include. The reviewer reproduced
  the evasion against the live function body, and noted that neither `[6/7]` fixture
  covers that shape because both are single-directive lines.
- **Why it is worth a note:** it widens a guard beyond the width its own banner claims to
  pin, which is the same class of defect the `[6/7]` controls exist to prevent.
- **Not fixed here, deliberately.** Out of scope for this gap closure by operator
  decision, and the guard is not currently being evaded by any file in the tree.
- **Resolve at:** the **next phase that touches `tests/check_includes.sh`**. The fix is to
  anchor the exclusion to a whole line (allowing the `grep -n` line-number prefix and a
  trailing comment) and to add the evasion shape as a third `[6/7]` control that must
  produce a hit.
- **Also recorded here so they are not lost:** `30-REVIEW.md` carries four **Info**
  findings — a `uint32_t` telemetry step counter with a reachable wrap; the audio-thread
  telemetry contract Phase 35 will cross; ~40 lines of duplicated input functors between
  test invariants 4 and 5; and a miscounted helper reference in a file banner. None is
  planned; `30-REVIEW.md` is the record.
