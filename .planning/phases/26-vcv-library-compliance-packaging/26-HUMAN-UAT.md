---
status: partial
phase: 26-vcv-library-compliance-packaging
source: [26-VERIFICATION.md]
started: 2026-07-09T02:37:49Z
updated: 2026-07-09T02:37:49Z
---

## Current Test

[awaiting human testing]

## Tests

### 1. Push commits and confirm CI is green on all three OS legs
expected: All three legs (ubuntu-latest, macos-latest, windows-latest) report SUCCESS with 50 doctest cases / ~2.6M assertions passed, 0 failed — matching the local macOS result (50 cases, 2,615,027 assertions, 0 failures). Push the unpushed local commits (b550c86..HEAD) to origin/main and observe the GitHub Actions `test` workflow run to completion.
result: [pending]

### 2. Audit minRackVersion=2.0.0 API surface
expected: Either (a) all six version-sensitive Rack API symbols (ParamQuantity::getScaledValue, Module::isBypassed, createIndexSubmenuItem, drawLayer self-illumination, Window::getLastFrameDuration, Widget::addChildBelow) resolve against the Rack 2.0.0 SDK, validating the floor, or (b) minRackVersion is raised to the lowest version actually confirmed. Only two of six were checked during execution (getLastFrameDuration, createIndexSubmenuItem).
result: [pending]

## Summary

total: 2
passed: 0
issues: 0
pending: 2
skipped: 0
blocked: 0

## Gaps
