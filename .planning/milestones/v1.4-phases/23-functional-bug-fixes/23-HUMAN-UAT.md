---
status: partial
phase: 23-functional-bug-fixes
source: [23-VERIFICATION.md]
started: 2026-06-15
updated: 2026-06-15
---

## Current Test

[awaiting human testing — final build installed + flushed, hash 1531f89; fully quit & relaunch Rack before testing]

## Tests

### 1. BUG-03 — phase dot tracks the trace in free-running mode with swing set
expected: With NO clock patched (free-running) and swing set to Medium or Heavy from the context menu, the phase dot sits ON the trace/output (straight, un-warped) — it does NOT race ahead of the waveform. (Fix: displaySwingFraction stores the effective gated value `isClocked ? swingFrac : 0.5f`.)
result: [pending]

### 2. BUG-04 — patch load survives malformed/corrupt JSON without crashing
expected: Load a `.vcv` patch whose Analog LFO `spreadSeed0` (or `spreadSeed1`) has been hand-edited to a non-hex value like `"zzzz"` (or emptied). Rack loads the patch WITHOUT crashing; the module keeps a valid seed (falls back to the existing/constructor seed) and runs normally. (Fix: non-throwing `parseSeedHex` behind the `json_is_string` guard; seeds committed only when both parse.)
result: [pending]

## Summary

total: 2
passed: 0
issues: 0
pending: 2
skipped: 0
blocked: 0

## Gaps
