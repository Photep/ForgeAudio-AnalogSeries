---
phase: 26
slug: vcv-library-compliance-packaging
status: secured
threats_open: 0
threats_total: 8
threats_closed: 8
asvs_level: 1
created: 2026-07-09
---

# SECURITY — Phase 26: vcv-library-compliance-packaging

**Audit date:** 2026-07-09
**ASVS level:** L1 (default)
**Disposition:** SECURED — 8/8 threats resolved (6 mitigated + verified, 2 accepted + logged)
**Implementation files:** unmodified (read-only audit)

This audit verifies each declared threat against the *implemented code*, not against
plan intent or summary claims. Every mitigation below was confirmed by a grep/build
check against the working tree.

---

## Threat Verification

| Threat ID | Category | Disposition | Status | Evidence (verified in tree) |
|-----------|----------|-------------|--------|-----------------------------|
| T-26-01 | Tampering | mitigate | CLOSED | `plugin.json`: tags `["Low-frequency oscillator","Waveshaper"]` are canonical VCV whitelist spellings; slug `ForgeAudio-AnalogSeries` + module slug `ForgeAnalogLFO` match `[A-Za-z0-9_-]+`; no trademarked synth/brand strings in name/brand/description/module fields |
| T-26-02 | Spoofing | mitigate | CLOSED (reachability deferred) | All three URLs (`authorUrl`/`pluginUrl`/`sourceUrl`) = `https://github.com/Photep/ForgeAudio-AnalogSeries`, byte-identical to `git remote origin`. URL-correctness half satisfied. HTTP-200 half is a documented deferral (repo intentionally PRIVATE until Phase 28 / PUB-01) — see Accepted/Deferred Risks |
| T-26-03 (drift) | Tampering | mitigate | CLOSED | `src/dsp/MathConst.hpp:12` `kPi = 3.14159265358979323846` compiled to bits `0x400921FB54442D18` — **bit-for-bit match confirmed** to M_PI. `grep -rn M_PI src/ tests/ tools/` = zero hits. `forge::kPi` present in all 5 consumers (Waveshape 5, DriftEngine 6, LfoCore 2, AnalogLFO 12, test_extraction 2) |
| T-26-03 (artifact) | Information Disclosure | mitigate | CLOSED | `res/fonts/` holds ONLY `JetBrainsMonoNL-Regular.ttf` + `OFL.txt`; no `Barell`/`FoundationLogo` anywhere under `res/`. `.vcvplugin` listing (26-04-SUMMARY) confirms same in the built artifact; no secrets in DISTRIBUTABLES (`Makefile:13-16` = res, LICENSE*, NOTICES*, presets) |
| T-26-04 | Tampering | mitigate | CLOSED | Generator `tools/capture_golden.cpp:54-55` constructs `forge::LfoCore` directly and calls ONLY `core.seed(...)` — `grep setSpreadSeed tools/capture_golden.cpp` = zero. Replay `tests/test_golden.cpp:92-93` (`replayGoldenDriftOff`) mirrors it: `core.seed` only, no `setSpreadSeed`. Portable Xoroshiro + libm path only |
| T-26-05 | Tampering | mitigate | CLOSED | `Makefile:60-62` `capture` target compiles with `$(TEST_CXXFLAGS)` = the same `-std=c++17 -O2 -g -Isrc -I tests -Wall -Wextra -ffp-contract=off` (Makefile:38) used by `test` (Makefile:44-46). Windows CI leg uses the identical flag string incl. `-ffp-contract=off`, no `-ffast-math` (`.github/workflows/test.yml:38`) |
| T-26-06 | Tampering | mitigate | CLOSED | No hand-rolled packaging in tree: `make dist` is provided by `../Rack-SDK/plugin.mk` (Makefile has no custom zip/tar target). 26-04-SUMMARY confirms verification via `tar --zstd -tvf` (zstd-tar), not `unzip` |
| T-26-CI | Elevation of Privilege | accept | CLOSED (logged) | `.github/workflows/test.yml`: checkout + build + run only; no `secrets.*` references, no `permissions:` escalation, no external actions beyond `actions/checkout@v4`. Default read-only `GITHUB_TOKEN` suffices. See Accepted Risks |
| T-26-SC | Tampering | accept | CLOSED (logged) | No package-manager installs anywhere in `.github/workflows/test.yml` or `Makefile` (no npm/pip/cargo). Toolchain is OS/SDK-provided (make, g++, jq, zstd, tar). No supply-chain surface. See Accepted Risks |

---

## Accepted / Deferred Risks (log)

### T-26-CI — GitHub Actions token scope (ACCEPTED)
The Rack-free CI matrix (`.github/workflows/test.yml`) only checks out the repo, builds
the doctest binary, and runs it on ubuntu/macos/windows. It references no secrets, requires
no elevated `permissions:` block, and uses only the pinned `actions/checkout@v4`. The default
read-only `GITHUB_TOKEN` is sufficient. **Accepted:** no least-privilege gap.

### T-26-SC — Supply chain (ACCEPTED)
No `npm install` / `pip install` / `cargo` / third-party package fetch exists in the phase.
The fixture generator is in-repo C++ (`tools/capture_golden.cpp`) compiled by the local/CI
toolchain; packaging uses OS/SDK tools. **Accepted:** no supply-chain surface to slopcheck.

### T-26-02 — URL HTTP-200 reachability (DEFERRED to Phase 28 / PUB-01)
The *URL-correctness* half of T-26-02 is CLOSED: all three manifest URLs equal the
operator-owned repo, byte-identical to `git remote origin`. The *reachability* half
(`curl` HTTP 200) currently returns 404 because the repo is intentionally PRIVATE through
the IP-hardening phases. This is deliberate roadmap sequencing, not a manifest defect.
**Deferred:** Phase 28 public flip must re-run reachability to close the remaining half.

---

## Follow-up CI change (informational — no threat surface change)
The Windows leg statically links the test binary
(`.github/workflows/test.yml:38`: `-static -static-libgcc -static-libstdc++`). This only
bundles the MinGW runtime (libstdc++/libgcc/libwinpthread) into `test.exe` so it launches
without those DLLs on PATH. Same libm, same `-ffp-contract=off` — golden determinism and FP
flag parity (T-26-05) are unaffected. **No new attack surface; no threat mapping required.**

---

## Unregistered Flags
None. The SUMMARY files (`## Threat Surface` in 26-03, `## Threat Mitigations Applied` in
26-04) map only to registered threat IDs. No new attack surface appeared during
implementation that lacks a threat mapping.

---

## Auditor notes
- Implementation files were NOT modified. Only this SECURITY.md was written.
- kPi bit-identity was independently confirmed by compiling the literal and reading its
  IEEE-754 bit pattern (`0x400921FB54442D18`), not by trusting the code comment.
- Spread-path neutralization was verified in BOTH the generator and the replay (the plan's
  named landmine), not just one side.

---

## Audit Trail

## Security Audit 2026-07-09
| Metric | Count |
|--------|-------|
| Threats found | 8 |
| Closed | 8 |
| Open | 0 |

Register authored at plan time (all four PLAN.md files carried `<threat_model>` blocks);
auditor ran in verify-mitigations mode (no retroactive STRIDE scan). All 6 mitigate-disposition
threats confirmed present in the implementation; 2 accept-disposition threats logged. One
half-threat (T-26-02 reachability) deferred by design to Phase 28.
