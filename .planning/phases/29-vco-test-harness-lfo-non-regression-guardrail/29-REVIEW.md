---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
reviewed: 2026-07-28T00:00:00Z
depth: standard
files_reviewed: 12
files_reviewed_list:
  - src/dsp/VcoCore.hpp
  - src/vco_compile_canary.cpp
  - src/dsp/FROZEN.sha256
  - tests/VcoBlockDriver.hpp
  - tests/test_vco_harness.cpp
  - tests/Sha256.hpp
  - tests/test_lfo_guardrail.cpp
  - tests/check_canary.sh
  - tests/check_frozen.sh
  - tests/check_includes.sh
  - Makefile
  - .github/workflows/test.yml
findings:
  critical: 3
  warning: 10
  info: 5
  total: 18
status: issues_found
---

# Phase 29: Code Review Report

**Reviewed:** 2026-07-28
**Depth:** standard
**Files Reviewed:** 12
**Status:** issues_found

## Summary

Phase 29 ships a Rack-free VCO seam plus three shell guards and an in-repo SHA-256
tripwire. Two things are genuinely solid and were verified rather than assumed:

- **`tests/Sha256.hpp` is correct.** I differentially tested it against system
  `shasum -a 256` at every padding boundary (55/56/57/63/64/65/119/120/127/128
  bytes) and against the million-`a` FIPS vector. Every digest matched. The K
  table, H init, message schedule, round function and length encoding are all
  right. There is no bug in the hash core.
- **All three guards pass today, and `make test` is green** (64 cases, 2 615 099
  assertions). The three manifests (`FROZEN.sha256`, `tests/golden/SHA256SUMS`,
  and the literals in `test_lfo_guardrail.cpp`) all agree with the files on disk.

The problem is not what the guards report — it is what they *cannot* report. All
three are **fail-open**: each one is a green light whose scope is a hand-written
list or a constant-folded expression, with no mechanism that notices when the
scope shrinks to nothing. I built working bypasses for three of them and confirmed
each yields `PASS` / `exit 0`:

1. VCO code reaching the shipped LFO translation unit through one transitive hop —
   all three guards green (CR-01).
2. Unguarding `src/AnalogLFO.cpp` by deleting its manifest line while editing it —
   `check_frozen.sh` green (CR-02).
3. An in-class `static constexpr` array indexed at runtime in `VcoCore.hpp` — the
   exact construct that got v2.0.0 rejected — leaving **no symbol at all** in the
   canary's `-O3` object, so the MinGW link leg has nothing to fail on, and
   `check_canary.sh` reports PASS (CR-03).

This is the same failure class the phase's own `check_includes.sh` banner
diagnoses about `check_docs.sh`: *"An unwired guard is worse than no guard: it is
a guard plus a false belief."* Three of these guards are wired but scoped to
something narrower than their stated contract, which produces the same false
belief with more ceremony.

Two deliberate design choices flagged as out-of-scope in the brief
(`vco_compile_canary.cpp`'s unused symbol, `VcoBlockDriver.hpp`'s intentional
duplication, `step()` returning `0.f`) are respected and not reported.

---

## Critical Issues

### CR-01: `check_includes.sh` [1/7] misses any transitive VCO leak into the shipped LFO build graph

**File:** `tests/check_includes.sh:77-151`

**Issue:** The D-06 boundary — the stated single most damaging failure mode of
this milestone — is enforced by grepping a **hardcoded 25-file list** for
**direct** `#include` lines matching `Vco|MorphBlep`. It never follows an include
edge and it never covers a file that is not literally in `LFO_SCAN`. Since any new
header under `src/dsp/` is neither in the list nor VCO-named, one intermediate hop
defeats the gate entirely.

Reproduced on a scratch copy of the tree. Adding an unpinned helper header and
one include line to the (deliberately unpinned, per the script's own banner)
`src/plugin.cpp`:

```cpp
// src/dsp/PluginHelper.hpp  (new, unpinned, not in LFO_SCAN)
#pragma once
#include "dsp/VcoCore.hpp"
namespace forge { inline float leak2() { VcoCore c; VcoInputs i; return c.step(i); } }
```
```cpp
// src/plugin.cpp  (append)
#include "dsp/PluginHelper.hpp"
```

Result — VCO code is now in the shipped plugin's registration translation unit and
every gate is green:

```
check_includes EXIT=0    ([1/7] "OK: 25 LFO-side files scanned, zero VCO includes")
check_frozen   EXIT=0    (plugin.cpp is deliberately not pinned)
check_canary   EXIT=0
```

The negative control in [6/7] does not catch this: its synthetic fixture uses a
*direct* `#include "dsp/VcoCore.hpp"`, which is the one shape the detector does
handle. It validates the regex, not the scope.

**Fix:** Invert the scan from an allowlist to a denylist so it is self-completing,
and resolve one level of indirection. Minimum viable change:

```bash
# Derive the LFO side instead of listing it: everything under src/ and tests/
# that is NOT explicitly VCO-side. A new file is covered the moment it lands.
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
)
LFO_SCAN=()
while IFS= read -r f; do
	rel="${f#${ROOT}/}"
	# VCO's own headers are the other side of the boundary.
	case "${rel}" in src/dsp/Vco*.hpp|src/dsp/MorphBlep.hpp) continue ;; esac
	skip=0
	for a in "${VCO_SIDE_ALLOW[@]}"; do [[ "${rel}" == "${a}" ]] && skip=1; done
	[[ "${skip}" -eq 1 ]] || LFO_SCAN+=("${f}")
done < <(find "${ROOT}/src" "${ROOT}/tests" "${ROOT}/tools" \
	-type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) | sort)
```

Then add a transitive pass: for every quoted `dsp/*.hpp` an LFO-side file
includes, recurse into that header and re-run `detect_vco_includes` on it (a
`while` worklist with a `seen` set is ~15 lines). Extend the [6/7] negative
control to cover the **two-hop** case, not just the direct one — otherwise the
control keeps validating a shape the bug does not use.

---

### CR-02: `check_frozen.sh` has no coverage floor — deleting a manifest line silently unguards a shipped file

**File:** `tests/check_frozen.sh:128-152`

**Issue:** The gate iterates whatever lines happen to be in `src/dsp/FROZEN.sha256`
and asserts nothing about how many there should be. The banner promises the gate
"hard-fails the moment any of them changes without a deliberate manifest bump" —
but *removing* a file from the manifest is strictly easier than bumping its digest,
and produces a full green with no signal whatsoever. The `(15 pinned entries
checked)` line is printed, never compared.

Reproduced on a scratch copy — delete one line and edit the shipped LFO shell in
the same commit:

```bash
grep -v 'src/AnalogLFO.cpp' src/dsp/FROZEN.sha256 > f2 && mv f2 src/dsp/FROZEN.sha256
printf '\n// silently unguarded edit\n' >> src/AnalogLFO.cpp
bash tests/check_frozen.sh   # -> EXIT=0, "PASS: frozen-source gate clean"
```

The [3/3] negative control does not catch this either: it is hardcoded to
`src/dsp/MathConst.hpp`, so it only fires if that one specific entry is the one
that vanishes. Every other entry can be dropped invisibly.

`tests/golden/SHA256SUMS` in [2/3] has the identical hole.

**Fix:** Pin the expected coverage set, not just the digests. Two complementary
guards:

```bash
# 1. A floor on entry count, bumped deliberately alongside the manifest.
FROZEN_EXPECTED_ENTRIES=15
GOLDEN_EXPECTED_ENTRIES=6
...
if [[ "${frozen_count}" -ne "${FROZEN_EXPECTED_ENTRIES}" ]]; then
	note_fail "${FROZEN_REL} has ${frozen_count} entries, expected ${FROZEN_EXPECTED_ENTRIES}. A file was REMOVED from the manifest — that unguards it silently, which is strictly worse than a digest mismatch. ${BUMP_HINT}"
fi

# 2. Completeness: every file the LFO closure actually contains must be pinned.
for f in "${ROOT}"/src/dsp/*.hpp "${ROOT}"/src/AnalogLFO.cpp; do
	rel="${f#${ROOT}/}"
	case "${rel}" in src/dsp/Vco*.hpp|src/dsp/MorphBlep.hpp) continue ;; esac
	grep -qF -- "  ${rel}" "${ROOT}/${FROZEN_REL}" \
		|| note_fail "${rel} exists under src/dsp but is NOT pinned in ${FROZEN_REL}"
done
```

Guard (2) also closes the "new unpinned header" half of CR-01.

---

### CR-03: The compile canary feeds only compile-time constants, so the in-class `static constexpr` failure class is constant-folded away before it can reach the linker

**File:** `src/vco_compile_canary.cpp:65-90`

**Issue:** This is the phase's central claim — that the CI MinGW leg is "the ONLY
gate that catches the in-class `static constexpr` failure class" (canary banner
lines 11-18; `test.yml:41-51`) — and it does not currently hold, because the
canary supplies no runtime-varying **data** to the seam.

`VcoInputs in;` takes its NSDMI defaults (`pitchCV = 0.f`, `morph = 0.f`, …) and
only `sampleTime` / `sampleRate` are assigned, both from literals. The one
runtime-derived value, `reps = (i & 3) + 1`, is the **loop trip count** — it
preserves how many times `step()` is called, but nothing inside `step()` depends
on it. Every index, branch and table lookup a future VCO `step()` performs on
`in.*` is therefore a compile-time constant and folds at `-O2`/`-O3`.

The banner's stated rationale is precisely inverted:

> "A compile-time-constant count would let the compiler fold the loop away; then
> this translation unit emits nothing, the VCO headers are odr-used by nothing…"

The loop folds away regardless, because `step()` returns a literal. Disassembling
the current canary at `-O3` (`c++ -std=c++11 -O3 -Isrc -c src/vco_compile_canary.cpp`)
shows the loop, the `step()` calls and `core.seed()` all eliminated; the only
surviving call is `forge::DriftEngine::setSpreadSeed`, which lives in an **LFO**
header. The canary object currently odr-uses **zero** symbols originating in
`dsp/VcoCore.hpp`.

Demonstrated directly by injecting the exact landmine into `VcoCore.hpp`:

```cpp
static constexpr float kTable[4] = {0.f, 1.f, 2.f, 3.f};  // C++11: DECLARATION only
float step(const VcoInputs& in) {
	...
	return kTable[((int)(in.pitchCV) & 3)];   // runtime index -> should odr-use kTable
}
```

At `-O3` the object contains **no `kTable` symbol of any kind** (at `-O0` it does:
`l__ZN5forge7VcoCore6kTableE.const`). `bash tests/check_canary.sh` returns
`EXIT=0` and prints `PASS`. There is nothing left for MinGW's linker to fail to
resolve, so the CI leg would be green on the same construct that got v2.0.0
rejected.

Caveat, stated honestly: measured with Apple clang 16 on arm64, because that is
what is available here. The banner is right that clang cannot reproduce the *link*
failure. But the defect proven above is **not** a clang quirk — it is constant
propagation of a literal `0.f` into an array index after inlining, which GCC
performs identically at `-O2`+, and the CI win-x64 leg compiles at `-O3`
(`test.yml:81`). The odr-use is destroyed at the compiler, before the linker is
ever consulted.

Note also that `check_canary.sh` [2/5] cannot detect this: it greps `nm` for the
**probe** symbol, which is emitted even when the body has been folded to
`return 0.f`. Its failure message ("the translation unit emits no code, so it
odr-uses nothing") describes a property it does not actually test.

**Fix (canary):** make the DSP inputs runtime-derived from the parameter, so no
index or branch inside `step()` can be folded:

```cpp
float vcoCompileCanaryProbe(int i) {
	VcoCore core;
	core.seed(0x1234ULL, 0x5678ULL);
	core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);

	VcoInputs in;
	in.sampleTime = 1.f / 44100.f;
	in.sampleRate = 44100.f;

	// LOAD-BEARING: every field the seam may index or branch on must be
	// RUNTIME-derived. A literal here lets -O3 constant-fold the index and the
	// odr-use disappears before the MinGW linker ever sees it — which is the one
	// thing this file exists to prevent. Do not replace these with constants.
	in.pitchCV   = (float)(i & 7) - 4.f;
	in.coarse    = (float)((i >> 3) & 3);
	in.fine      = (float)((i >> 5) & 3);
	in.morph     = (float)(i & 15) / 15.f;
	in.character = (float)((i >> 4) & 15) / 15.f;
	in.drift     = (float)((i >> 8) & 15) / 15.f;

	const int reps = (i & 3) + 1;
	float acc = 0.f;
	for (int n = 0; n < reps; ++n) {
		in.pitchCV += (float)n * 0.125f;   // keeps each iteration distinct
		acc += core.step(in);
	}
	return acc + core.tel.displayPhase + core.tel.freqHz;
}
```

**Fix (guard):** strengthen `check_canary.sh` [2/5] to assert the property it
claims, by compiling a synthetic header carrying the landmine and requiring the
`-O3` object to retain a reference to it. That converts [2/5] from "the probe
exists" into "the seam is still odr-used", and gives this failure class the
observed-red validation every other section of this phase demanded of itself:

```bash
# [2b/5] The canary must odr-use the seam, not merely exist.
cat > "${TMP}/odr_probe.hpp" <<'EOF'
#pragma once
struct CanaryOdrProbe {
	static constexpr float kCanaryOdrTable[4] = {0.f, 1.f, 2.f, 3.f};
	float pick(int i) const { return kCanaryOdrTable[i & 3]; }
};
EOF
cat > "${TMP}/odr_probe.cpp" <<'EOF'
#include "odr_probe.hpp"
float canaryOdrCall(int i);
float canaryOdrCall(int i) { CanaryOdrProbe p; return p.pick(i); }
EOF
"${CXX_BIN}" -std=c++11 -O3 -I"${TMP}" -c "${TMP}/odr_probe.cpp" -o "${TMP}/odr.o"
if nm "${TMP}/odr.o" | grep -q 'kCanaryOdrTable'; then
	echo "  OK: a runtime-indexed in-class static constexpr survives -O3 as a symbol"
else
	note_fail "at -O3 this compiler folds a runtime-indexed in-class static constexpr away entirely. The canary's inputs must be runtime-derived (see src/vco_compile_canary.cpp) or the MinGW link leg covers nothing."
fi
```

---

## Warnings

### WR-01: `check_includes.sh` [1/7] silently skips listed-but-missing files and reports a fabricated scan count

**File:** `tests/check_includes.sh:79-80, 150`

**Issue:** `detect_vco_includes` does `[[ -f "${f}" ]] || continue`, so a file that
is in `LFO_SCAN` but no longer on disk is skipped without a word. The success
message then prints `${#LFO_SCAN[@]}` — the **array length**, not the number of
files actually opened. Verified: with `src/dsp/Waveshape.hpp` temporarily moved
aside, the gate still prints `OK: 25 LFO-side files scanned, zero VCO includes`.
Renaming any LFO file silently reduces coverage while the output claims otherwise.

**Fix:**

```bash
scanned=0
detect_vco_includes() {
	local f
	for f in "$@"; do
		if [[ ! -f "${f}" ]]; then
			note_fail "LFO_SCAN lists ${f#${ROOT}/} but it does not exist — the scan silently covers less than it claims. Remove it from LFO_SCAN or restore the file."
			continue
		fi
		scanned=$((scanned + 1))
		...
	done
}
# ...and report the real number:
echo "  OK: ${scanned} LFO-side files scanned, zero VCO includes"
```

Note `scanned` must not be incremented inside a subshell for this to work — call
the function directly rather than in `$( )`, or write the count to a temp file.

### WR-02: `check_canary.sh` [4/5] negative controls report success for any compile failure, not the intended one

**File:** `tests/check_canary.sh:178-196`

**Issue:** `check_negative_control` treats a non-zero compiler exit as proof that
"the C++11 pedantic gate rejected the C++17-ism", and discards the diagnostic with
`2>/dev/null`. Any unrelated failure — a typo in a fixture, a renamed header, a
missing include path — produces the same green. Verified by breaking
`dsp/VcoCore.hpp` with a bogus `#include`: all four controls printed
`OK: … rejected by the C++11 pedantic gate` even though not one of them reached
the C++17 construct.

This section exists specifically to be "validated by an observed red rather than
by never having been anything but green" (banner, line 21). A control that cannot
distinguish the intended red from an accidental one does not deliver that.

**Fix:** assert the diagnostic text, and assert the fixture compiles clean at
C++17 so a broken fixture is caught:

```bash
check_negative_control() {
	local nc_label="$1" nc_file="$2" nc_mode="$3" nc_expect="$4"
	# Sanity: the fixture must be VALID C++17 — otherwise a typo masquerades as a red.
	if ! "${CXX_BIN}" -std=c++17 -fsyntax-only -I"${ROOT}/src" "${nc_file}" 2>/dev/null; then
		note_fail "${nc_label}: fixture does not compile even at -std=c++17, so its rejection at C++11 proves nothing"
		return
	fi
	if "${CXX_BIN}" "${STRICT_FLAGS[@]}" "${nc_file}" 2>"${TMP}/nc_err.txt"; then
		[[ "${nc_mode}" == "hard" ]] \
			&& note_fail "${nc_label}: ACCEPTED by the C++11 pedantic gate" \
			|| echo "  INFO: ${nc_label}: accepted by this compiler (informational only)"
	elif grep -qiE "${nc_expect}" "${TMP}/nc_err.txt"; then
		echo "  OK: ${nc_label}: rejected for the expected reason"
	else
		note_fail "${nc_label}: rejected, but NOT for the C++17 reason this control tests:"
		sed 's/^/    /' "${TMP}/nc_err.txt"
	fi
}

check_negative_control "namespace-scope 'inline constexpr' variable" "${TMP}/nc_inline_constexpr.cpp" hard 'inline variable'
check_negative_control "'if constexpr' statement"                    "${TMP}/nc_if_constexpr.cpp"     hard 'constexpr if|if constexpr'
check_negative_control "'std::clamp' call"                           "${TMP}/nc_std_clamp.cpp"        hard 'clamp'
check_negative_control "'[[maybe_unused]]' attribute"                "${TMP}/nc_maybe_unused.cpp"     info 'maybe_unused|attribute'
```

### WR-03: `check_includes.sh` [4/7] ODR guard is evaded by a next-line brace

**File:** `tests/check_includes.sh:223-224`

**Issue:** The regex `^[[:space:]]*struct[[:space:]]+Inputs[[:space:]]*[{:]`
requires `{` or `:` on the same line. Verified evasion:

```cpp
namespace forge {
struct Inputs
{
	float x = 0.f;
};
}
```

`grep -nE '^[[:space:]]*struct[[:space:]]+Inputs[[:space:]]*[{:]'` reports no
match, so a second `forge::Inputs` — the R-9 trap this section is dedicated to —
passes. Allman brace style is common enough that this is not hypothetical.

**Fix:** drop the brace requirement and exclude forward declarations explicitly:

```bash
inputs_decls="$(grep -rnE '^[[:space:]]*struct[[:space:]]+Inputs([[:space:]]*[{:]?[[:space:]]*)?$|^[[:space:]]*struct[[:space:]]+Inputs[[:space:]]*[{:]' \
	--include='*.hpp' --include='*.h' --include='*.cpp' "${ROOT}/src" \
	| grep -vE 'struct[[:space:]]+Inputs[[:space:]]*;' || true)"
```

### WR-04: `check_includes.sh` [5/7] hasher-placement constant check is case-sensitive

**File:** `tests/check_includes.sh:268`

**Issue:** The comment on line 254 claims the constant check "is unrestricted and
would still catch an implementation smuggled in under any filename." It is
case-sensitive: `grep -rn '0x6a09e667'` does not match `0x6A09E667u`, which is how
a large fraction of published SHA-256 implementations write it. Verified.

**Fix:**

```bash
# -i: the same constant is commonly written 0x6A09E667.
const_hits="$(grep -rni '0x6a09e667' "${ROOT}/src" || true)"
```

Consider also matching a second constant (`0xbb67ae85`) so a partial table cannot
slip through.

### WR-05: `check_frozen.sh` manifest parsing drops an unterminated last line and is not CR-safe on the CRLF checkout its banner exists to support

**File:** `tests/check_frozen.sh:133, 166`

**Issue:** Two distinct defects in the same `while read -r expected relpath` loops.

(a) A manifest whose final line lacks a trailing newline loses that entry
silently. Verified:

```bash
printf 'aaa one\nbbb two' > /tmp/nl.txt
while read -r a b; do echo "read: $a $b"; done < /tmp/nl.txt
# -> read: aaa one          (the "bbb two" entry is gone)
```

Both manifests currently end with `0x0a`, so this is latent — but it is exactly
CR-02's failure mode reachable by an editor setting rather than an edit.

(b) The script's entire CR-normalization apparatus (banner lines 54-63,
`hash_norm`) exists so the gate is "correct on a CRLF checkout too". But the
manifests themselves are text files with no `.gitattributes` protecting them, so
on a Windows checkout `relpath` carries a trailing `\r`, `[[ -f "${ROOT}/${relpath}" ]]`
is false for every entry, and the gate reports **every pinned file as MISSING**.
The stated Windows-correctness property does not hold. Not hit in CI today (the
guards run only on the `toolchain-gate` ubuntu job), but `make guards` is
documented as a local developer command.

**Fix:** strip CR while reading and force a final newline:

```bash
while read -r expected relpath || [[ -n "${expected:-}" ]]; do
	relpath="${relpath%$'\r'}"
	expected="${expected%$'\r'}"
	[[ -z "${expected}" ]] && continue
	...
done < <(tr -d '\r' < "${ROOT}/${FROZEN_REL}"; echo)
```

Apply the same to the `SHA256SUMS` loop at line 166. While there, skip comment
lines (`[[ "${expected}" == \#* ]] && continue`) — a `#` line currently produces a
spurious "pinned file is MISSING" failure.

### WR-06: `check_includes.sh` [7/7] accepts a guard script that is only *mentioned* in the workflow

**File:** `tests/check_includes.sh:345`

**Issue:** `grep -qF -- "${grel}" "${WORKFLOW}"` matches anywhere in the file,
including YAML comments and `name:` strings. A script named in a comment but never
invoked — precisely the `check_docs.sh` situation this section was written to
prevent — would report `OK: … wired into CI`. The section's premise is that
mentioning is not running; its implementation only checks mentioning.

**Fix:** require the reference to appear in an executable position:

```bash
if grep -qE "^[[:space:]]*(run:|-)?[^#]*\b(bash|sh)[[:space:]]+${grel//./\\.}\b" "${WORKFLOW}"; then
```

Better still, parse the `run:` blocks only, or assert the script name appears in
the output of a `--dry-run`-style CI lint. At minimum, exclude comment lines:

```bash
grep -v '^[[:space:]]*#' "${WORKFLOW}" | grep -qF -- "${grel}"
```

### WR-07: `make guards` duplicates the CI guard list with nothing checking they agree, and CI never runs the target

**File:** `Makefile:99-107`, `.github/workflows/test.yml:98-117`

**Issue:** `GUARD_SCRIPTS` in the Makefile and the three `run: bash tests/check_*.sh`
steps in `test.yml` are independent hardcoded lists of the same three scripts. CI
invokes the scripts directly and never runs `make guards`, so the Makefile list
has no execution coverage at all. `check_includes.sh` [7/7] validates the workflow
side only.

Net effect: a guard added in a later phase and wired into CI (satisfying [7/7])
but omitted from `GUARD_SCRIPTS` leaves `make guards` — the command the Makefile
banner advertises as "one local command for every standing guard this milestone
adds" — silently incomplete. This is the P-5 lesson reappearing one level up.

**Fix:** make CI run the Makefile target so there is a single list, and extend
[7/7] to cover it:

```yaml
      - name: LFO non-regression guard suite (D-05/D-06/D-07)
        run: make guards
```

```bash
# [7/7] addition — the Makefile target must carry every guard too.
MAKEFILE="${ROOT}/Makefile"
if ! grep -qE "^GUARD_SCRIPTS[[:space:]]*:?=.*${grel//./\\.}" "${MAKEFILE}"; then
	note_fail "${grel} is not in the Makefile GUARD_SCRIPTS list — \`make guards\` silently skips it."
fi
```

### WR-08: `Sha256::update()` silently discards data after `hex()` has been called

**File:** `tests/Sha256.hpp:43-47`

**Issue:** `update()` returns early when `finalized` is set, with no error, no
assert and no return value. A caller who streams, reads an intermediate digest,
then streams more gets the digest of the **first segment only** and no indication
anything was dropped. Verified:

```
h.update("abc"); d1 = h.hex();
h.update("def"); d2 = h.hex();
d1 == d2  -> YES  (d2 = ba7816bf…  which is sha256("abc"), not sha256("abcdef"))
```

The header's contract paragraph (lines 38-40) documents `hex()` idempotency but
says nothing about this. For a file whose stated job is being a tripwire — where a
silently-short digest is the exact failure mode that would launder a regression —
a silent wrong answer is the wrong default.

**Fix:** make it loud, or make it impossible:

```cpp
void update(const unsigned char* data, size_t len) {
	// Feeding a finalized hasher is a caller bug that would silently produce the
	// digest of a PREFIX of the message. Never ignore it quietly.
	assert(!finalized && "Sha256::update() after hex(): the digest is already sealed");
	if (finalized || data == 0 || len == 0)
		return;
	...
}
```

Add a test case asserting the behaviour (whichever is chosen) so it is pinned.

### WR-09: the CR-stripping branch of `hashFileImpl` has zero test coverage and no caller

**File:** `tests/Sha256.hpp:201-230, 263-265`

**Issue:** `sha256HexFileLfNormalized()` is called in exactly one place —
`test_lfo_guardrail.cpp:180`, on a **missing** file, which returns at the
`is_open()` check before any hashing happens. The
`stripCarriageReturns == true` code path (lines 214-224) is therefore never
executed by any test, and the function has no production caller anywhere in the
repo. Confirmed by grep across `src/`, `tests/`, `tools/`.

The header presents this function as the answer to pitfall P-3 (Windows CRLF
checkouts), and `check_frozen.sh`'s banner rests on the same normalization idea in
shell. The C++ half of that story is untested code.

**Fix:** either wire it up or prove it. A direct test costs five lines:

```cpp
TEST_CASE("lfo guardrail: LF-normalized file hash ignores CR bytes") {
	// Write two scratch files with identical content modulo line endings.
	{ std::ofstream a("build-test/nc_lf.txt",   std::ios::binary); a << "one\ntwo\n"; }
	{ std::ofstream b("build-test/nc_crlf.txt", std::ios::binary); b << "one\r\ntwo\r\n"; }
	CHECK(forge::sha256HexFileLfNormalized("build-test/nc_lf.txt")
	   == forge::sha256HexFileLfNormalized("build-test/nc_crlf.txt"));
	// And prove the RAW variant still distinguishes them, or the guard is vacuous.
	CHECK(forge::sha256HexFile("build-test/nc_lf.txt")
	   != forge::sha256HexFile("build-test/nc_crlf.txt"));
	// Cross-check: on an LF file the two entry points must agree.
	CHECK(forge::sha256HexFileLfNormalized("build-test/nc_lf.txt")
	   == forge::sha256HexFile("build-test/nc_lf.txt"));
}
```

If the function genuinely has no consumer until plan 29-04 lands, note that in the
header — an unused, untested hashing variant is exactly the kind of thing a later
phase will trust without checking.

### WR-10: VCO detection is keyed on a filename convention that nothing enforces

**File:** `tests/check_includes.sh:76, 88-92`; `tests/check_canary.sh:211-215`

**Issue:** Four separate guards — [1/7] leak detection, [2/7] Rack-free, [3/7]
include hygiene, and [5/5] the D-08 growth rule — all key off the same
case-sensitive filename tokens `Vco*` / `MorphBlep`. A future VCO header named
anything else (`dsp/OscCore.hpp`, `dsp/PitchChain.hpp`, `dsp/Antialias.hpp`) is
invisible to **all four simultaneously**, and every one of them still reports PASS
because they iterate a glob that simply matches fewer files.

`check_canary.sh` [5/5] guards against forgetting to *include* a Vco-named header;
nothing guards against forgetting to *name* it Vco-something. Additionally, on
macOS's case-insensitive filesystem an `#include "dsp/vcocore.hpp"` resolves fine
but does not match `VCO_TOKEN='(Vco|MorphBlep)'`.

**Fix:** state the naming rule as a gate, not a convention. Add to
`check_canary.sh`, after the header set is collected:

```bash
# Every header reachable from the VCO seam must be Vco*-named, or the four guards
# that key on that token silently stop covering it.
seam_includes="$(grep -oE '#include[[:space:]]*"dsp/[A-Za-z0-9_]+\.hpp"' \
	"${ROOT}/src/dsp/Vco"*.hpp | grep -oE 'dsp/[A-Za-z0-9_]+\.hpp' | sort -u)"
for inc in ${seam_includes}; do
	b="$(basename "${inc}")"
	case "${b}" in
		Vco*|MorphBlep.hpp) ;;
		# Frozen LFO headers the VCO is allowed to consume (D-05, one-way).
		DriftEngine.hpp|MathConst.hpp|RackCompat.hpp|Waveshape.hpp) ;;
		*) note_fail "${inc} is pulled into the VCO seam but is neither Vco*-named nor an allowed frozen shared header. The Vco* glob in check_includes.sh [1-3/7] and check_canary.sh [5/5] will not cover it." ;;
	esac
done
```

Make `VCO_TOKEN` matching case-insensitive (`grep -niE`) at the same time.

---

## Info

### IN-01: `head -1` under `set -o pipefail`, contradicting the sibling script's own documented rule

**File:** `tests/check_canary.sh:111`
**Issue:** `nm … | grep -E … | awk … | head -1` runs under `set -o pipefail`.
`check_frozen.sh:98` explicitly documents avoiding this: *"awk (rather than head)
consumes the whole line, which avoids a SIGPIPE under `set -o pipefail`."*
Harmless here only because the pipeline sits inside an `echo` argument, so its
status never reaches `set -e`. It is a trap for the next person who copies the line.
**Fix:** `… | awk 'NR==1 {print $2, $3}'`.

### IN-02: canary comment-stripping handles `//` only, not `/* */`

**File:** `tests/check_canary.sh:73, 209`
**Issue:** `grep -v '^[[:space:]]*//'` removes line comments but not block comments,
so `/* #include "dsp/MorphBlep.hpp" */` would satisfy the [5/5] growth-rule check.
[3/5] would catch the resulting compile failure today, so this is defence-in-depth
rather than a live hole.
**Fix:** note the limitation in the comment, or strip block comments with
`sed 's|/\*[^*]*\*/||g'` before the greps.

### IN-03: `HASHER` is assigned three ways and used only for an echo

**File:** `tests/check_frozen.sh:101, 104, 107, 123`
**Issue:** All real hashing goes through `hash_stdin()`; `HASHER` exists purely for
the `using hasher: …` line. Harmless, but it reads like a command that might be
invoked and invites someone to start invoking it (which would bypass the
CR-normalization in `hash_norm`).
**Fix:** rename to `HASHER_LABEL` to make the intent unambiguous.

### IN-04: `check_negative_control` uses global variables in a function

**File:** `tests/check_canary.sh:179-181`
**Issue:** `nc_label`, `nc_file`, `nc_mode` are assigned without `local`, leaking
into the shell scope. Every other function in the three scripts that takes
arguments (`detect_vco_includes`) uses `local`.
**Fix:** `local nc_label="$1" nc_file="$2" nc_mode="$3"`.

### IN-05: `VcoBlockDriver::run()` does not guard a negative sample count

**File:** `tests/VcoBlockDriver.hpp:53-55`
**Issue:** `out.reserve(nSamples)` converts a negative `int` to an enormous
`size_t`, throwing `std::length_error` from deep inside the harness rather than
failing at the call site. Test-only code with no current negative caller, but the
harness is explicitly built to outlive Phase 29.
**Fix:**
```cpp
std::vector<float> run(int nSamples, const std::function<forge::VcoInputs(int)>& inputAt) {
	if (nSamples <= 0)
		return std::vector<float>();
	...
}
```

---

_Reviewed: 2026-07-28_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
