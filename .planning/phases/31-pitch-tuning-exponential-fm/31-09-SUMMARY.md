---
phase: 31-pitch-tuning-exponential-fm
plan: 09
subsystem: operator-uat
tags: [in-rack-uat, operator-signoff, install-freshness, whole-tree-byte-equality, milestone-guardrail, affordance-vs-behavior, deferred-routing]

# Dependency graph
requires:
  - phase: 31-pitch-tuning-exponential-fm
    plan: 04
    provides: "the four declared controls (COARSE, FINE, FM DEPTH, FM IN) on the throwaway panel with stock SDK widgets — the reason there is anything for an operator to move"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 06
    provides: "the behavioral proof of FM-02's bipolarity (inversion bit-exact, sign-difference observable, zero a bit-exact no-op) — which is why the operator's observation could be split cleanly into a closed behavior and an open affordance"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 08
    provides: "the phase gate: four local gates plus the three-OS matrix and the Windows link leg observed green by hash on 80fb90a, and the 13-item deferred register this plan appends to"
provides:
  - "an operator verdict on the pitch chain, recorded VERBATIM: approval WITH one observation"
  - "the milestone guardrail's own user-visible UAT evidence — the shipped Analog LFO confirmed visually and audibly unchanged in the same Rack session as the new VCO"
  - "install freshness proven by WHOLE-TREE byte equality (diff -r) rather than by three sampled facts, after the sampled facts were measured to be insufficient"
  - "a measured demonstration that two of the three prescribed freshness facts are satisfied by a stale artefact, including an identical dylib BYTE SIZE and identical export addresses"
  - "deferred register items 14 (FM DEPTH's affordance -> Phase 35) and 15 (a second Forge plugin in the plugins tree -> Phase 32's verification protocol)"
affects: [32-band-limiting, 35-shell-panel-display, 36-goldens-ci-library]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Prove an install fresh by WHOLE-TREE byte equality against the build output (diff -r), not by reading back a chosen set of facts — a sampled fact set is only as strong as its weakest discriminator, and this project has now measured a stale artefact passing two of three"
    - "A file SIZE check is not a freshness check: the stale and fresh dylibs were both 175,056 bytes and exported the same two symbols at the same two addresses"
    - "When operator feedback lands on a control, split it into BEHAVIOR (is it what the requirement says?) and AFFORDANCE (does the widget communicate it?) before routing — the two have different owners and only one was open"
    - "Verify a claimed in-house precedent before citing it. The obvious framing here ('the LFO's own attenuverters') was measured FALSE: all 10 LFO configParams are unipolar"
    - "Name the plugin DIRECTORY as well as the module when asking an operator to audition something, if more than one build of the same plugin family can sit in the plugins tree"

key-files:
  created:
    - ".planning/phases/31-pitch-tuning-exponential-fm/31-09-SUMMARY.md"
  modified:
    - ".planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md"

key-decisions:
  - "The operator's observation is recorded as an approval WITH an observation, never upgraded to an unqualified approval and never downgraded to a blocker"
  - "FM-02's bipolar BEHAVIOR is NOT reopened — no param range, default or sign changed anywhere; the requirement's own wording says bipolar and 31-06 proved it bit-exactly"
  - "The AFFORDANCE is routed to Phase 35 as register item 14, which also recovers the one 31-04 hand-off this register had dropped"
  - "The 'shipped LFO attenuverters are the precedent' framing was checked and REJECTED as false; the real precedent is role/size (five ForgeTrimpot CV-depth controls vs the VCO's full RoundBlackKnob)"
  - "src/AnalogVCO.cpp:39-42 was checked and deliberately LEFT ALONE — it already says the FM widget's physical form is unsettled and Phase 35's"
  - "No requirement marked complete: this plan's evidence CORROBORATES PITCH-02/03 and FM-01/02/03, which 31-06 already marked, and corroboration is recorded rather than re-marked"

requirements-completed: []

coverage:
  - id: D1
    description: "The install Rack loaded is genuinely fresh — proven by whole-tree byte equality against dist/, not by a sampled fact set, and re-confirmed after the operator's session"
    verification:
      - kind: integration
        ref: "diff -r dist/ForgeAudio-AnalogSeries '<installed>' -> NO DIFFERENCES, run twice (before the ask last session and again this session after the operator had Rack open); aggregate sha256 over all 18 installed files = 0d57b964b8cb21a8...; zero .vcvplugin archives in the plugins directory"
        status: pass
      - kind: integration
        ref: "the WHOLE installed directory was mv'd OUT of the plugins tree before anything was written, then rsync -a from a dist/ built out of a removed build/ and dist/; Rack not running before or during the swap (pgrep -x Rack -> none)"
        status: pass
  - id: D2
    description: "The three prescribed facts read back from the INSTALLED copy agree with the build tree and with the source"
    verification:
      - kind: integration
        ref: "installed plugin.json parses: version '2.0.1', 2 modules, modules[0]='ForgeAnalogLFO' FIRST, modules[1]='ForgeAnalogVCO' matching createModel<AnalogVCO, AnalogVCOWidget>(\"ForgeAnalogVCO\") at src/AnalogVCO.cpp:294 character for character"
        status: pass
      - kind: integration
        ref: "installed res/AnalogVCO.svg: 10 <rect>, 8 #2a2a30 markers, 766 bytes, sha256 2bb1349d73c2aed8...; installed plugin.dylib exports _modelAnalogLFO and _modelAnalogVCO (nm -gU), 175,056 bytes, sha256 5061619cbe45e53b..."
        status: pass
  - id: D3
    description: "The full expected-results block was presented BEFORE the verification steps and before the ask"
    verification:
      - kind: manual
        ref: "what-built, then all TEN expected-results items including the uncomfortable ones (crude aliased timbre, >5 V excursions, unlabelled placeholder panel, five-digit tooltips, flattened peaks under deep FM, the stall at extreme negative pitch, the clone behavior), then the nine how-to-verify steps, then stop — in one message, in that order"
        status: pass
    human_judgment: true
  - id: D4
    description: "The operator's reply is recorded VERBATIM, with the one observation routed to exactly one outcome"
    requirement: "FM-02"
    verification:
      - kind: manual
        ref: "reply transcribed verbatim: 'Approved - I was not expecting a bipolar knob for FM though'. Routed to deferred register item 14, owner Phase 35, with the behavior/affordance split explicit. Nothing paraphrased, nothing summarised away, nothing invented"
        status: pass
    human_judgment: true
  - id: D5
    description: "The milestone guardrail's user-visible check passed — the shipped Analog LFO is unchanged in the same session as the new VCO"
    verification:
      - kind: manual
        ref: "step 9 covered by the approval; no LFO observation raised. Corroborated headlessly by the six bit-exact LFO .f32 goldens replaying inside make test (81 / 2,618,053 / 0) and by check_frozen.sh inside make guards"
        status: pass
    human_judgment: true
  - id: D6
    description: "The regression floor holds on the tree the operator auditioned"
    verification:
      - kind: unit
        ref: "make test -> 81 test cases / 2,618,053 assertions / 0 failed — the phase floor met EXACTLY"
        status: pass
      - kind: integration
        ref: "make guards -> 'guard suite: PASS'; make strict -> 'strict C++11 gate: PASS'"
        status: pass
  - id: D7
    description: "No source file, param range, default or sign was changed in response to the observation"
    verification:
      - kind: integration
        ref: "the plan's only commit touches .planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md; src/, res/, tests/, plugin.json, src/dsp/, FROZEN.sha256 and src/AnalogLFO.cpp are all absent from the diff"
        status: pass

# Metrics
duration: 12min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 09: Put the Phase in Front of a Human Summary

**The operator approved the pitch chain in Rack and confirmed the shipped LFO unchanged in the same session — with one observation, recorded verbatim, which turned out to land exactly on the one deferred item this phase's register had dropped; and the install they judged was proven fresh by whole-tree byte equality after the plan's own three-fact check was measured to be satisfied by a stale artefact two ways out of three, including an identical dylib byte size.**

## Performance

- **Duration:** ~12 min of executor time across two sessions, separated by the blocking operator checkpoint
- **Tasks:** 2 (Task 1 verification-only; Task 2 a blocking human-verify checkpoint, now resolved)
- **Repository files modified:** 1 (`deferred-items.md` — the routing this plan's own Task 2 action requires)
- **Completed:** 2026-07-30

## The operator's verdict — VERBATIM

> "Approved - I was not expecting a bipolar knob for FM though"

**This is an approval WITH one observation.** It is recorded as exactly that: not upgraded to an
unqualified approval, and the observation not treated as a blocker. The operator did not enumerate
which of the nine steps they performed; the reply is a global approval, and **the parts of it that are
load-bearing for this phase are the ones the wording actually supports** — see "What the approval does
and does not evidence" below, which is deliberately conservative about that.

### The ordering was honoured, so the silence is exposure rather than omission

The single message presented, in this order: the `what-built` summary → **all TEN expected-results
items** → the nine `how-to-verify` steps → the ask. The ten included every uncomfortable one: the
crude, buzzy, aliased-on-purpose timbre; the unconditioned output exceeding five volts (~5.55 V
measured ceiling); the deliberately ugly eight-grey-square placeholder panel; the five-significant-digit
tooltips; the peaks **flattening** rather than going silent under deep FM; the stall to near-DC at
extreme negative pitch; and every VCO instance being a bit-identical clone.

So the absence of any timbre, level, panel-ugliness, tooltip, flattening, stall or clone complaint is
an **absence of complaint, not an absence of exposure** — the same discipline Phase 30's sign-off was
taken under, and the reason that sign-off is usable evidence at all.

**Item 3 of the block explicitly told the operator the FM depth control is "a full knob for now" and
that "whether it becomes a scalloped trimpot is that phase's decision."** The operator raised the
widget anyway. That is worth stating plainly: they were exposed to the item and still found it
surprising enough to mention, which is a stronger signal than a cold observation would have been.

---

## The observation, routed

Exactly one observation was raised. It is routed to **exactly one** outcome: **the phase's deferred
register, item 14, owner Phase 35.** The substantive judgement is that it splits cleanly in two, and
only one half is open.

### Half one — the BIPOLARITY is closed. Not a defect, not discretionary, not reopened.

| Source | What it fixes |
|---|---|
| `REQUIREMENTS.md` **FM-02** | *"A dedicated **bipolar** attenuverter sets FM depth"* — **bipolar is the requirement's own word** |
| **D-07** | `-1..+1`, linear taper, default `0`, displayed `-100%..+100%` |
| **D-06** | full clockwise = 1.0 octave per volt — the FM jack as a second V/OCT input |
| **31-06 invariant 6** | `fmAtten = -1` inverts `+1` **bit-exactly** against the negated shift; the two signs produce **different** blocks; `fmAtten = 0` is a **bit-exact no-op** |

D-07 is worth singling out, because it **already anticipated this exact confusion**. It exists to
record that *"the shipped LFO's controls named 'atten' are unipolar attenuators, not attenuverters"*
while FM-02 and roadmap criterion 3 both specify bipolar for the VCO. The operator's surprise is the
predicted one, and the decision that predicted it also resolved it.

**Consequence, stated because it is the one thing that could have gone wrong here: no param range,
default or sign was changed.** Adjusting FM DEPTH to `0..1` would have broken an approved requirement
on the strength of a remark about how the control *reads*. `src/AnalogVCO.cpp:136` is byte-unchanged:

```cpp
configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
```

### Half two — the AFFORDANCE is genuinely open, and it is Phase 35's

A plain full-size knob communicates nothing about *centre-is-off* or *counter-clockwise-inverts*. The
operator turned it, found bipolar behavior, and reported that the widget had not told them to expect
it. **That is an affordance report and it is correct.** CONTEXT.md **D-16** already scopes the panel as
a throwaway replaced wholesale in Phase 35, `31-04-SUMMARY.md` records stock SDK widgets only, and the
expected-results block itself said the widget choice is Phase 35's. So this observation **confirms a
known open decision rather than surprising anyone** — and it is now the first piece of real operator
evidence behind it.

### ⚠ The register had already LOST this item once

`31-04-SUMMARY.md` § "Deferred / not done" item 6 handed **three** items forward to 31-08: COARSE
octave detents, **the FM depth control's physical form (full knob vs scalloped trimpot)**, and patch
persistence for the four new controls. The register carries the first as **item 6** and folded the
third into **item 5** — and **omitted the second entirely**. Measured before this plan appended
anything:

```
$ grep -ni 'trimpot\|physical form\|full knob\|widget' \
    .planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md
(no output)
```

**So the operator independently rediscovered, by eye and in about a minute, the exact item that fell
through the hand-off.** That is the cheapest possible demonstration of this phase's own recurring
thesis — an unregistered deferral gets rediscovered cold — and it is recorded in item 14 rather than
quietly fixed.

### ⚠ The obvious precedent framing is FALSE, and citing it would have been wrong

The tempting way to write item 14 is *"the shipped Analog LFO's own attenuverters are the in-house
precedent for how a bipolar control should read."* **That was checked against the source this session
and it is false.**

```
$ grep -c 'configParam' src/AnalogLFO.cpp
10
$ grep -n 'configParam([A-Z_]*, *-' src/AnalogLFO.cpp
(no output)
```

**The shipped LFO has ZERO bipolar params.** All ten have a non-negative minimum — nine explicit `0.f`
plus `configParam<RateParamQuantity>(RATE_PARAM, 0.01f, 20.f, ...)`. So **the VCO's FM DEPTH is the
first bipolar control anywhere in this plugin**, and there is **no in-house visual language for bipolar
to match**. Phase 35 must *invent* one, not copy one. That strengthens the observation rather than
weakening it.

**What the real in-house convention is — and the VCO diverges from it.** The convention is about
**role and size**. All five of the LFO's CV-depth controls are `ForgeTrimpot`, in a source-labelled
`// CV trimpots` row at `y = 108.50`: `MORPH_ATTEN_PARAM`, `CHARACTER_ATTEN_PARAM`,
`DRIFT_ATTEN_PARAM`, `FM_ATTEN_PARAM`, `PHASE_OFFSET_ATTEN_PARAM`. Its four secondary params are
`ForgeKnobSecondary`; RATE is `ForgeKnobHero`.

**The sharpest form of the operator's observation, therefore:**

| | Shipped Analog LFO | New Analog VCO |
|---|---|---|
| Param name | `FM_ATTEN_PARAM`, `"FM Depth"` | `FM_ATTEN_PARAM`, `"FM Depth"` — **identical** |
| Range | `0..1` unipolar | `-1..+1` **bipolar** |
| Widget | **`ForgeTrimpot`** (small, in the CV row) | **`RoundBlackKnob`** (`src/AnalogVCO.cpp:274-275`) |
| Relative weight | visibly a modulation-depth trimpot | **the same widget class as MORPH, CHARACTER, COARSE and FINE** |

On the throwaway panel there is **no size or role hierarchy at all**, so a modulation-depth control is
presented with exactly the weight of a primary tune control — while its identically-named sibling on
the shipped module is a small trimpot. The operator's surprise is corroborated by an actual convention;
just a convention about role/size rather than about bipolarity.

### Candidate remedies handed to Phase 35 (none decided here)

1. **A visually distinct attenuverter widget** — the role/size precedent points at a trimpot or a
   dedicated bipolar widget rather than a full-size knob.
2. **A centre detent or centre indicator** — a notch, a twelve-o'clock tick, or a centre-origin arc
   fill growing left-negative / right-positive, so *centre-is-off* is legible without turning it.
3. **Clearer labelling** — a `-/+` or `−100..0..+100` legend, which the throwaway panel cannot carry
   because it has no text at all.

### Why nothing was fixed here — three independent grounds

- This plan's prohibitions forbid modifying any repository file, and Task 1's criterion requires a
  clean tree. (The register append is the exception Task 2's own action mandates — see Deviations.)
- The remedy is a **widget and art decision**, definitionally Phase 35's. D-08 records that the Forge
  Noir knob structs are **local to `src/AnalogLFO.cpp`**, the shipped module's translation unit, so
  building a Forge-styled attenuverter here would mean extracting components out of live released
  source — the one thing this milestone's cleanest guardrail position rests on not doing.
- Changing the range, default or sign instead of the widget would break FM-02.

### The source comment was checked and deliberately LEFT ALONE

The resume instruction asked whether `res/AnalogVCO.svg` or `src/AnalogVCO.cpp` carry any comment that
now reads as a settled choice about the FM depth widget. **Neither does.** `src/AnalogVCO.cpp:39-42`
already reads:

```
// The FM depth control's PHYSICAL form is deliberately NOT settled here. Whether
// it ends up a full knob or a scalloped trimpot is Phase 35's call, because that
// phase owns the real layout and the whole control budget. This phase declares
// the param, gives it a stock widget and a marker rect, and stops there.
```

Already provisional, already correctly owned, already naming the trimpot alternative. And
`res/AnalogVCO.svg` contains **no comments at all** (`grep -n '<!--'` → nothing). So the file was
**checked and not edited**: leaving a true comment alone is as much a part of comment-truth discipline
as correcting a false one, which is 31-04's own rule. The throwaway panel was not restyled.

---

## Task 1 — the install, re-verified this session

**Resolved plugins directory (discovered, not guessed):**

```
/Users/mrcbrown/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries
```

Resolved from the machine architecture (`arm64`); the sibling `plugins-mac-x64` tree has no Forge
entry.

**The flush was a WHOLE-DIRECTORY replacement, and that is the load-bearing claim.** The installed
directory was `mv`'d **OUT** of the plugins tree before anything was written, then repopulated with
`rsync -a` from a `dist/` built out of a **removed** `build/` and `dist/`. No file from a previous
version could survive, because no previous file was in the destination when the copy began. There are
**zero `.vcvplugin` archives** anywhere in the plugins directory, so Rack cannot re-extract a stale
archive at launch. Rack was **not running** before or during the swap (`pgrep -x Rack` → none).

### Freshness re-verified after the operator's session

The operator has had Rack open since the install, so the check was re-run at the end of this plan:

```
$ diff -r dist/ForgeAudio-AnalogSeries \
    "/Users/mrcbrown/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries"
(no output — exit 0)
```

**Whole-tree byte equality, still holding.** Aggregate digest over all **18** installed files:
`0d57b964b8cb21a8f751096ddfd0733177f4cd4fe0283b48d51c5d7e4d0aed29`. Rack loading the plugin left it
byte-unchanged.

### The three prescribed facts, re-read from the INSTALLED copy

| # | Fact | Reading |
|---|---|---|
| 1 | manifest parses, 2 modules, shipped slug first | `version '2.0.1'`; `modules[0]='ForgeAnalogLFO'`, `modules[1]='ForgeAnalogVCO'` |
| 2 | panel present with the expected rect count | `res/AnalogVCO.svg` — **10** `<rect>`, **8** `#2a2a30` markers, 766 bytes, sha256 `2bb1349d73c2aed8…` |
| 3 | binary exports the VCO model symbol | `nm -gU` → `_modelAnalogLFO` **and** `_modelAnalogVCO`; 175,056 bytes, sha256 `5061619cbe45e53b…` |

Fact 1's slug matches the source's factory string **character for character** —
`src/AnalogVCO.cpp:294`:

```cpp
Model* modelAnalogVCO = createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO");
```

### ⚠ THE FINDING: two of the three prescribed facts are satisfied by a STALE artefact

This is the most important technical result of the plan, and it is a **measurement**, not a worry. The
pre-flush artefact was preserved rather than deleted, and re-interrogated this session:

| Check | Stale artefact | Fresh install | Discriminates? |
|---|---|---|---|
| Manifest: 2 modules at `2.0.1`, both slugs | **`2.0.1`, `['ForgeAnalogLFO','ForgeAnalogVCO']`** | same | **NO — fact 1 PASSES on the stale copy** |
| Dylib exports both model symbols | **`_modelAnalogLFO` @ `0x14380`, `_modelAnalogVCO` @ `0x14388`** | **the same two symbols at the same two addresses** | **NO — fact 3 PASSES on the stale copy** |
| Dylib byte size | **175,056** | **175,056** | **NO — byte-for-byte the same size** |
| Dylib sha256 | `4ec31c0fb640b405…` | `5061619cbe45e53b…` | yes |
| `res/AnalogVCO.svg` rect count | **6** | **10** | **YES — the only prescribed fact that discriminated** |

**So the plan's three-fact freshness check would have passed a stale plugin on two of its three legs,
and a size check on the binary would have passed it too.** The single discriminator was the rect
count — one number, in one file, out of a three-fact set explicitly designed for independence.

This is why freshness was escalated to **whole-tree `diff -r` byte equality** rather than reported as
satisfied. It is also the second time in two phases that this project's install-freshness safeguard has
been measured weaker than it reads: Phase 30 measured the false negative reaching **through** the
safeguard, and this plan measured the replacement safeguard's own sampling being **two-thirds
insufficient**. Both findings arose only because the stale artefact was **kept** instead of discarded.

Preserved at:
`/private/tmp/.../scratchpad/preflush-backup/ForgeAudio-AnalogSeries.stale`

**Preconditions confirmed rather than assumed:** working tree clean; `HEAD` = `250471e`; source tree
identical to `80fb90a` — `git diff --name-only 80fb90a..HEAD` returns only four `.planning/` files —
which is the commit whose four local gates, three-OS matrix and Windows link leg 31-08 observed green
**by hash** (run `30511183170`).

---

## What the approval does and does not evidence

Deliberately conservative, because the reply is a global "Approved" and did not enumerate steps.

**Evidenced by the approval:**

- **The module appears and the four new controls are audibly live** (step 1-6, D-16's whole reason for
  declaring them in this phase rather than deferring to the panel phase). The observation itself is
  positive proof of engagement with FM DEPTH specifically — the operator could only be surprised by
  bipolar behavior by **encountering** it, which means the FM jack, the depth control and the inversion
  were all reached and heard.
- **The milestone guardrail's user-visible check (step 9, T-31-04):** the shipped Analog LFO looks and
  sounds identical in the same session as the new VCO. The block told the operator that a difference
  here *"is the most important thing you could report"*; none was reported.
- **No timbre, output-level, panel, tooltip, flattening, stall or clone complaint** — each of which was
  stated explicitly first.

**NOT evidenced, and not claimed:**

- **Which specific steps were performed.** The operator did not say, and the plan's criterion asks the
  summary to record which nine steps they actually did. **It cannot be recorded, so it is not** — no
  step is attributed to them beyond what the wording supports. Inferring a per-step pass from a global
  approval would be exactly the invention the plan prohibits.
- **Any numeric confirmation.** Nothing in the reply confirms the ~1-cent tracking, the ten-octave
  COARSE travel or the ±100-cent FINE extremes *as numbers*. Those are the headless gate's, measured in
  31-05/31-06; this session evidences that they are musically right to an ear, which is the only thing
  an audition can add.

---

## Register items appended

`.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md` grew from 13 items to **15**.

**Item 14 — FM DEPTH's affordance → Phase 35.** Carries the verbatim observation, the
behavior-is-closed / affordance-is-open split with FM-02 and D-06/D-07 cited, the three candidate
remedies, the correction of the false LFO-attenuverter precedent, the verified role/size precedent, the
note that this register had dropped the item, and the explicit instruction that Phase 35 changes how
the control **reads** and never what it **does**.

**Item 15 — a second Forge plugin shares the plugins tree → Phase 32's verification protocol.** An
**executor finding, not an operator observation**, and labelled as such in the register. The plugins
directory contains two Forge directories:

| Directory | Manifest slug | Version | Modules | Dated |
|---|---|---|---|---|
| `ForgeAudio-AnalogSeries` | `ForgeAudio-AnalogSeries` | `2.0.1` | `ForgeAnalogLFO`, `ForgeAnalogVCO` | Jul 30 (this install) |
| `ForgeAudio` | `ForgeAudio` | `2.0.0` | `ForgeAudioLFO` | **Feb 14** |

**It does not weaken the freshness proof** — Rack keys plugins by manifest slug, the two slugs differ,
the module slugs differ (`ForgeAudioLFO` vs `ForgeAnalogLFO`), so neither can shadow or be
re-extracted over the other, and the `diff -r` equality is unaffected. **What it qualifies** is step 9:
the module browser holds **two** Forge LFO entries, so "the Analog LFO is unchanged" has a subject that
is inferred rather than pinned. The approval and step 1's pairing of "Analog LFO" with the newly added
VCO in the same browser session make the intended reading almost certain — but the ambiguity is
recorded rather than smoothed over, because the alternative is a guardrail sign-off whose subject is
assumed. The remedy is one line of protocol for the next in-Rack session: **name the plugin directory
as well as the module.** Nothing outside `ForgeAudio-AnalogSeries` was touched — removing a plugin from
the operator's own Rack installation is not this plan's to do.

---

## Gate results on the audited tree

| Gate | Required | Observed |
|---|---|---|
| `make test` | 81 / 2,618,053 / 0 | **81 test cases / 2,618,053 assertions / 0 failed** — met EXACTLY |
| `make guards` | PASS | **`guard suite: PASS`** |
| `make strict` | PASS | **`strict C++11 gate: PASS`** |
| `diff -r dist vs installed` | no differences | **no differences** (re-run post-session) |

The suite figure matters beyond the floor: the six bit-exact LFO `.f32` goldens replay **inside**
`make test`, and `check_frozen.sh` runs **inside** `make guards`. So the operator's step-9 judgement
and the headless byte-level guardrail check agree, from two independent directions, on the same tree.

## Milestone guardrail compliance

- **`src/AnalogLFO.cpp`, `res/AnalogLFO.svg`, `src/dsp/` and `src/dsp/FROZEN.sha256` are absent from
  this plan's diff.** The only file touched is `.planning/.../deferred-items.md`.
- **The frozen headers** — `LfoCore.hpp`, `Waveshape.hpp`, `RackCompat.hpp` — were **read only**, never
  written. `src/AnalogLFO.cpp` was read this session solely to verify the precedent claim (10
  `configParam` calls, five `ForgeTrimpot` placements) before citing it, and that read is why the false
  framing was caught.
- **The operator's approval IS the guardrail's own UAT evidence.** Step 9 put the live,
  library-published module in front of a human in the same Rack session as the new one, after being
  told a difference would be the most important possible report. None came.
- **No param range, default or sign changed** anywhere in the plugin.
- **No `tests/` file touched**, so nothing was weakened to accommodate anything.

## Threat mitigations applied

| Threat ID | Disposition | Evidence |
|---|---|---|
| **T-31-31** (a stale install passing as fresh) | mitigate — **and the mitigation was measured, then strengthened** | Whole-directory `mv`-out-then-`rsync` flush; whole-tree `diff -r` byte equality, run twice; 18-file aggregate digest; zero `.vcvplugin` archives. **The prescribed three-fact check was measured to pass on the stale artefact two ways out of three, with an identical dylib byte size and identical export addresses** — recorded above so the next phase does not inherit a sampling check believing it independent |
| **T-31-04** (the shipped LFO regressing) | mitigate | Step 9's operator confirmation, plus the six goldens replaying inside `make test` and `check_frozen.sh` inside `make guards`, on the audited tree. **Qualified honestly by register item 15** — two Forge LFO browser entries exist, so the audited subject is inferred rather than pinned |
| **T-31-03** (a runaway accumulator poisoning the patch) | mitigate | Step 8 drove the extremes at full FM depth including a patch save and reload; no hang, stutter, crash, silence or downstream poisoning reported, against a block that named all four failure modes first |
| **T-31-08** (a degenerate seed hanging Rack on patch open) | mitigate | No seeding changed this phase; step 8's save-and-reload is where that hang would appear and none was reported |
| **T-31-32** (a summarised or invented observation) | mitigate | The reply is transcribed verbatim, character for character, including its lowercase "though" and its single hyphen. The one observation is recorded as an observation, the approval as an approval, and neither is upgraded into the other. **The steps performed are recorded as UNKNOWN rather than inferred.** Register item 15 is explicitly labelled an executor finding so it cannot be misread as the operator's |
| T-31-SC | accept | Zero registry packages; the install copied only this repository's own build output into the local plugins directory |

## Decisions Made

1. **The verdict is recorded as an approval WITH an observation.** Not upgraded, not downgraded. The
   plan's own prohibition against softening an observation cuts both ways: reporting this as
   "Approved" alone would lose the observation, and reporting it as a defect would lose the approval.
2. **FM-02's bipolarity is not reopened, and no range, default or sign changed.** The requirement says
   bipolar, D-06/D-07 fix the behavior, 31-06 proved it bit-exactly. A remark about how a control reads
   is not evidence against what it does.
3. **The affordance is routed to Phase 35 as register item 14 rather than acted on**, on three
   independent grounds (the plan's prohibitions; the remedy is Phase 35's widget/art decision and
   building a Forge-styled widget here would mean extracting components from live released source; and
   the alternative — changing the range — would break FM-02).
4. **The "LFO attenuverters are the precedent" framing was verified and REJECTED.** All 10 LFO
   `configParam` calls are unipolar, so the VCO's FM DEPTH is the plugin's first bipolar control and
   there is no in-house bipolar language to match. Citing the false precedent would have handed Phase
   35 a non-existent reference to copy. The **real** precedent — five `ForgeTrimpot` CV-depth controls
   versus the VCO's full `RoundBlackKnob` — is stronger and is what item 14 carries.
5. **`src/AnalogVCO.cpp:39-42` was checked and deliberately left unedited**, because it already says
   the widget's physical form is unsettled and Phase 35's, and names the trimpot alternative by name.
   The panel was not restyled.
6. **Install freshness was escalated to whole-tree byte equality** rather than reported as satisfied by
   the prescribed three facts, because two of the three were measured to pass on the stale artefact.
7. **The steps the operator performed are recorded as UNKNOWN.** The plan asks for them; the reply does
   not contain them; inventing a per-step pass from a global approval is exactly what the plan
   prohibits. The gap is recorded as a gap.
8. **No requirement marked complete.** See below.

## Deviations from Plan

### Justified prohibition exception

**1. [Rule 3 — Blocking] One repository file was modified, against the plan's "Do not modify any
repository file"**

- **Found during:** Task 2, at the routing step.
- **The conflict:** the plan's prohibitions say *"Do not modify any repository file in this plan"*, and
  Task 1's acceptance criterion requires a clean `git status --porcelain`. But **Task 2's own action
  requires** that each observation be routed to one of three outcomes, one of which is *"added to this
  phase's deferred register with a named owner phase"*, and its acceptance criterion repeats that an
  observation must be *"either resolved in this phase or written into the phase's deferred register with
  an owner."* An observation was raised, so the register write is **mandated by the same plan**.
- **Resolution:** the prohibition is read as scoped to what it protects — **source, resources, tests
  and the manifest**, i.e. the tree the operator just audited and Task 1 proved clean before installing.
  Writing the phase's own planning register is the prescribed output of Task 2, not a change to the
  audited artefact. **Task 1's clean-tree criterion was satisfied at Task 1's own time**, which is when
  it binds.
- **Scope actually taken:** one file, `.planning/phases/.../deferred-items.md`. `src/`, `res/`,
  `tests/`, `plugin.json`, `src/dsp/` and `src/AnalogLFO.cpp` are all absent from the diff, so the
  installed plugin the operator approved still corresponds byte-for-byte to the audited source.
- **Commit:** `235a9c8` (docs).

### Recorded gaps rather than worked around

**2. The nine performed steps cannot be recorded, because the reply does not name them**

- **Issue:** an acceptance criterion requires *"The summary records which of the nine steps the operator
  actually performed."* The reply is a global "Approved" plus one observation.
- **Action: recorded as unknown, with the specific inferences the wording does support enumerated
  separately** (see "What the approval does and does not evidence"), and the ones it does not support
  explicitly disclaimed. **Not worked around by inference.** The one thing the observation *does* prove
  positively is engagement with FM DEPTH and its inversion, since bipolar behavior can only surprise
  someone who reached it.

**3. Two of the plan's three freshness facts are non-discriminating — measured, not suspected**

- **Issue:** the plan describes the three facts as *"three independent facts"*. Two of them pass on the
  stale artefact, so the set's actual discriminating power is one fact, not three.
- **Action:** the stronger whole-tree check was used and the measurement recorded in full, with the
  stale artefact preserved. **This corrects a claim in the plan's own threat register (T-31-31), which
  rests on the three-fact readback.** Recorded rather than quietly substituted, so the next phase does
  not inherit a check believing it independent.

### Deliberate non-action

**4. No requirement marked complete**

- **PITCH-02, PITCH-03, PITCH-04, PITCH-05, FM-01, FM-02 and FM-03 are already `- [x]` / Complete**,
  marked by 31-05/31-06/31-07 on behavioral evidence. This plan **corroborates** PITCH-02, PITCH-03,
  FM-01, FM-02 and FM-03 by operator audition — an ear confirming that one volt of FM really is an
  octave, that COARSE glides, that FINE beats, and that centre is off and counter-clockwise inverts.
- **Corroboration is recorded, not re-marked.** Re-marking an already-complete requirement would add no
  information and would obscure which plan actually carries each requirement's evidence.
- **`requirements-completed: []`.** This plan evidenced no requirement that was not already evidenced,
  and marks nothing it did not evidence.

**5. The second `ForgeAudio` plugin directory was left in place** — the operator's machine, plausibly
deliberate (it is the pre-rename v2.0.0 slug), and outside this plan's remit. Recorded as item 15.

---

**Total deviations:** 1 justified prohibition exception (mandated by the same plan's Task 2), 2 recorded
gaps, 2 deliberate non-actions. **Impact on plan:** none on scope. Zero source change, zero param
change, zero test change.

## Issues Encountered

- **None blocking.**
- **The plan's three-fact freshness check is weaker than it reads** — see deviation 3. The strongest
  available form of the finding: the stale and fresh dylibs are **the same byte size (175,056)** and
  export **the same two symbols at the same two addresses** (`0x14380`, `0x14388`). Only the content
  hash and the panel's rect count told them apart.
- **A dropped hand-off was found by an operator rather than by a gate** — see item 14. 31-04 handed
  three items to 31-08 and the register received two. The gap had no tripwire, because a deferred item
  that never reached the register is invisible to every check that reads the register.
- **Two Forge LFO entries exist in the module browser** — see item 15. Not a defect in this
  repository, but a real ambiguity in how strongly a guardrail sign-off can be read.

## User Setup Required

None new. The plugin is installed and current at
`/Users/mrcbrown/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries`,
byte-identical to `dist/`. The only build dependency remains the locally pinned SDK at `../Rack-SDK`.

**Optional housekeeping, entirely the operator's call:** the stale
`plugins-mac-arm64/ForgeAudio` directory (v2.0.0, `ForgeAudioLFO`, Feb 14) is a separate plugin under
the pre-rename slug. It is harmless — different slug, cannot shadow the current install — but it puts a
second Forge LFO in the module browser. Removing it would make future in-Rack auditions unambiguous.
**Not done here**, because deleting a plugin from a personal Rack installation is not this plan's to do.

## Next Phase Readiness

- **Phase 31 is complete and operator-approved.** All 9 plans executed; the phase's user-facing half has
  been judged by a user.
- **Phase 32 (band-limiting) starts from a confirmed baseline, not an assumed one.** The crude, buzzy,
  aliased timbre was stated explicitly and drew **no complaint**, so Phase 32 inherits *"aliased as
  designed"* rather than *"possibly an unreported problem"* — the same position Phase 30 handed this
  phase, now held for a second phase.
- **Phase 32 also inherits two protocol obligations:** (a) register item 15 — **name the plugin
  directory as well as the module** in its own operator session; and (b) prove its install fresh by
  **whole-tree byte equality**, not by a sampled fact set, for the reason measured above.
- **Phase 34 inherits no reported level problem.** The >5 V unconditioned output (~5.55 V ceiling) was
  stated and not challenged. Register item 7's amplitude-fade alternative stays available to it, and the
  flattened-peak sound under deep FM was auditioned and not objected to.
- **Phase 35 inherits its first piece of real operator evidence** — register item 14, with the
  behavior/affordance split, three candidate remedies, and the corrected precedent (no bipolar language
  exists in the plugin yet; the role/size convention puts depth controls in trimpots). It also still
  inherits the bijective widget-to-rect geometry check from 31-04, re-runnable unchanged against the
  real panel.
- **The register now stands at 15 items**, of which item 1 is deliberately unowned (the shipped LFO's
  latent UB, a guardrail event) and item 4 is resolved.
- **No blockers.**

## Self-Check: PASSED

- `.planning/phases/31-pitch-tuning-exponential-fm/31-09-SUMMARY.md` — FOUND
- `.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md` — FOUND, 15 items
- Commit `235a9c8` — FOUND
- Installed plugin directory — FOUND, `diff -r` against `dist/` clean
- `src/`, `res/`, `tests/`, `plugin.json`, `src/dsp/`, `FROZEN.sha256`, `src/AnalogLFO.cpp` — absent
  from this plan's diff
- `configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f)` — byte-unchanged
- No file deletions in `235a9c8` (`git diff --diff-filter=D` empty)
- `make test` (81 / 2,618,053 / 0), `make guards` PASS, `make strict` PASS at HEAD

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
