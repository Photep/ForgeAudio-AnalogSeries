---
phase: 23-functional-bug-fixes
plan: 04
subsystem: process
tags: [audition, human-verify, checkpoint, clock-sync, ratio-alignment, gate]

# Dependency graph
requires:
  - phase: 23-functional-bug-fixes
    provides: "CURRENT (Wave-1-fixed, BUG-02-unchanged) plugin built + installed for in-Rack listening"
provides:
  - "Logged keep-current-vs-adopt-table audition decision in STATE.md ### Decisions (the Wave-C gate)"
affects: [23-05 (BUG-02 alignment change + cadence regression — parameterized on this decision)]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Audition-before-code human gate (research Pitfall 7 / SC3): a behavior-changing DSP fix is blocked until a human listening test logs a dated decision"
    - "Stale-install flush: shasum-compare fresh vs extracted-install dylib, rsync the dist payload, relaunch Rack — Rack loads the extracted dir, not the archive"

key-files:
  created: []
  modified:
    - .planning/STATE.md

key-decisions:
  - "DECISION: adopt-table — the current x1.5/÷1.5 cadence truncates mid-cycle and is not a desirable groove; adopt the BEATS_PER_ALIGN table (x1.5 → every 2 beats, ÷1.5 → every 3 beats)"

requirements-completed: [BUG-02 (audition half — gates the code half in 23-05)]
---

# 23-04 — x1.5/÷1.5 Audition Gate (BUG-02, SC3)

**Type:** `checkpoint:human-verify` (blocking). No DSP/source change — the only file touched is `.planning/STATE.md`.

## What happened

The defining gate of Phase 23. BUG-02 changes user-audible behavior, so an autonomous agent must not self-decide x1.5 desirability — a human in-Rack listening test decides.

1. **Built + installed the CURRENT plugin** (`make RACK_DIR=../Rack-SDK` + `make install`). The x1.5/÷1.5 alignment path lives in `RatioTable.hpp::shouldReset`, which Wave 1 did **not** touch — so the installed build reproduces the true unfixed BUG-02 behavior.
2. **Flushed the stale install** (Rack loads the extracted dir, not the archive): the installed extracted dylib was stale (`f29fc57`, prior day); synced the fresh `dist/ForgeAudio-AnalogSeries/` payload into the install dir; **hashes then matched (`3bb6fba`)** — confirmed before the audition.
3. **Operator auditioned** x1.5 and ÷1.5 against a steady clock after relaunching Rack.

## Decision (logged in STATE.md `### Decisions`)

> `- [Phase 23]: x1.5/÷1.5 audition — DECISION: adopt-table — rationale: in-Rack listening (operator, fresh-flushed CURRENT build, install hashes matched 3bb6fba before audition) confirmed the current cadence truncates mid-cycle — x1.5 retriggers every beat (chops ½ cycle) and ÷1.5 resets every 2 beats (truncates ⅓ cycle); the proposed BEATS_PER_ALIGN table (x1.5 → every 2 beats, ÷1.5 → every 3 beats) is preferred. Gates plan 23-05: apply the two-cell table swap (idx 8 → 2, idx 6 → 3) and pin with the deterministic cadence regression.`

**Outcome:** `adopt-table`. Plan 23-05 will apply the two-cell `BEATS_PER_ALIGN` swap (idx 8 → 2, idx 6 → 3) and pin the new cadence with the deterministic regression (`EXPECTED[15]` set to the adopt-table values).

## Acceptance criteria

- [x] STATE.md `### Decisions` contains a `[Phase 23]: x1.5 … DECISION: adopt-table` line.
- [x] Decision recorded AFTER auditioning the freshly-flushed CURRENT build (install hashes matched `3bb6fba` first).
- [x] No DSP/source/test file modified by this plan (only `.planning/STATE.md`).

## Self-Check: PASSED
