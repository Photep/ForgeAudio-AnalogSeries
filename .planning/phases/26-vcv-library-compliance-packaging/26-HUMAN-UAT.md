---
status: resolved
phase: 26-vcv-library-compliance-packaging
source: [26-VERIFICATION.md]
started: 2026-07-09T02:37:49Z
updated: 2026-07-09T03:00:00Z
---

## Current Test

[all tests complete]

## Tests

### 1. Push commits and confirm CI is green on all three OS legs
expected: All three legs (ubuntu-latest, macos-latest, windows-latest) report SUCCESS with 50 doctest cases / ~2.6M assertions passed, 0 failed — matching the local macOS result (50 cases, 2,615,027 assertions, 0 failures). Push the unpushed local commits (b550c86..HEAD) to origin/main and observe the GitHub Actions `test` workflow run to completion.
result: PASSED — CI run 28990864282 green on all three legs. macOS: 50 cases / 2,615,027 assertions, 0 failed. Ubuntu: 47 cases, 0 failed. Windows: 47 cases / 2,590,445 assertions, 0 failed (Windows/Ubuntu run 47 because the 3 drift-on golden cases are `#if defined(__APPLE__)`-gated by design in plan 26-03). Required an additional fix: the first push (run 28990684610) exposed a third CI root cause — the MinGW-built Windows `test.exe` exited 127 (missing runtime DLLs). Fixed by static-linking (`-static -static-libgcc -static-libstdc++`) in commit a0570d6.

### 2. Audit minRackVersion=2.0.0 API surface
expected: Either (a) all six version-sensitive Rack API symbols (ParamQuantity::getScaledValue, Module::isBypassed, createIndexSubmenuItem, drawLayer self-illumination, Window::getLastFrameDuration, Widget::addChildBelow) resolve against the Rack 2.0.0 SDK, validating the floor, or (b) minRackVersion is raised to the lowest version actually confirmed. Only two of six were checked during execution (getLastFrameDuration, createIndexSubmenuItem).
result: PASSED — all six symbols verified present in the Rack v2.0.0-tagged headers and CHANGELOG (getScaledValue is inherited from base rack::Quantity; the other five are declared directly in their v2.0.0 headers, most listed explicitly in the v2.0.0 changelog). All six were introduced at the v2.0.0 API baseline — none in a later 2.x minor. minRackVersion=2.0.0 is SAFE; no floor change required.

## Summary

total: 2
passed: 2
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps
