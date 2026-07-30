#!/usr/bin/env bash
#
# check_includes.sh — the D-06 dependency-direction audit.
#
# WHY THIS EXISTS
# ---------------
# v2.0 builds a VCO next to a module that is already published in the VCV
# library. The single most damaging way that could go wrong is not a bug in the
# VCO — it is VCO code silently entering the LFO's build graph. One misplaced
# #include is enough: it changes what the shipped module compiles to, and every
# existing test still passes because the tests exercise behavior, not topology.
#
# This gate makes dependency direction mechanical rather than conventional:
#
#   VCO code may depend on frozen shared headers.   (allowed, one-way)
#   LFO code may NEVER depend on VCO code.          (this script enforces it)
#
# It also carries two rules that live nowhere else:
#
#   * R-9, the ODR trap. A second `struct Inputs` in namespace forge compiles
#     cleanly in any translation unit that sees only one of the two definitions,
#     links without a diagnostic on Apple clang, and is a genuine
#     cross-translation-unit ODR violation with undefined behavior. It is the
#     exact failure class that got v2.0.0 rejected by the library, in a new
#     costume. Section 4 asserts there is exactly one, in src/dsp/LfoCore.hpp.
#     (The VCO's own input POD is deliberately named `forge::VcoInputs`.)
#
#   * The hasher placement rule. This milestone uses SHA-256 as an INTEGRITY
#     TRIPWIRE, not as a security control. It belongs to the test scope only
#     (tests/Sha256.hpp). Section 5 fails if any hashing implementation appears
#     under src/, because src/ is the shipped C++11 build graph and a hash
#     implementation there would be dead weight in the released binary and an
#     invitation to mistake a tripwire for a security primitive.
#
# Enforces:
#   [1/7] no LFO translation unit includes a VCO file, DIRECTLY OR TRANSITIVELY.
#         The LFO side is DERIVED (everything under src/, tests/ and tools/ that
#         is not an explicitly named VCO-side file), not hand-listed, so a new
#         file is covered the moment it lands.
#   [2/7] VCO headers are Rack-free, carrying ONE documented exact-path
#         exemption: the quoted path "dsp/RackCompat.hpp", which is this
#         repository's own Rack-FREE compatibility shim and not a Rack SDK
#         header at all. Every other [Rr]ack-named include still fails, and
#         [6/7] proves that on every invocation.
#   [3/7] VCO headers include only dsp/ siblings and standard headers
#   [4/7] R-9 ODR guard — exactly one forge::Inputs, in src/dsp/LfoCore.hpp
#   [5/7] no hashing implementation under src/
#   [6/7] NEGATIVE CONTROLS — this section validates TWO detectors rather than
#         one. For the section-1 VCO-leak detector it demonstrably reports both
#         a DIRECT and a TWO-HOP synthetic violation. For the section-2
#         Rack-free detector it proves BOTH directions of the documented
#         exemption: a genuine Rack SDK include is still caught, and the one
#         exempted shim path is not flagged. They run the SAME functions those
#         sections run: a control that exercises different code than the guard
#         proves nothing, and a control that only covers the direct shape
#         validates the regex rather than the scope.
#   [7/7] guard wiring (P-5) — every tests/check_*.sh is referenced by
#         .github/workflows/test.yml, or is on a documented exemption list
#
# Needs no compiler and no Rack SDK, runs from any working directory, and leaves
# no artifacts. Returns 0 (PASS) only when every group passes.

set -euo pipefail

# Resolve repo root relative to this script so it runs from anywhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }

# Scratch dir for the synthetic fixture in [6/7]; removed on exit so a run never
# leaves residue in the working tree.
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

# ---------------------------------------------------------------------------
# THE DETECTOR.
#
# Takes a list of files and echoes one "path:line:text" record per include
# directive whose target names a VCO file. Empty output means clean.
#
# This is deliberately a function rather than an inline grep: [6/7] runs THIS
# function over a synthetic violating fixture, so the negative control validates
# the exact code path [1/7] depends on. A control that re-implements the check it
# is validating proves only that two greps agree.
# ---------------------------------------------------------------------------
VCO_TOKEN='(Vco|MorphBlep)'

# The detector runs inside $( ), i.e. a SUBSHELL, so it cannot set `fail` or
# increment a counter for its caller — anything it learns has to come back
# through the filesystem. These two logs are how: one records every file
# actually OPENED, the other every file that was handed to the detector but does
# not exist. The caller resets them, then reads them after the call.
#
# The count matters. This section used to report ${#LFO_SCAN[@]} — the array
# LENGTH — as "N files scanned", while `[[ -f ]] || continue` silently skipped
# anything missing. With one LFO header moved aside the gate still printed
# "OK: 25 LFO-side files scanned". Renaming any LFO file reduced coverage while
# the output claimed otherwise.
SCAN_OPENED="${TMP}/scan_opened.txt"
SCAN_MISSING="${TMP}/scan_missing.txt"
: > "${SCAN_OPENED}"
: > "${SCAN_MISSING}"

detect_vco_includes() {
	local f
	for f in "$@"; do
		if [[ ! -f "${f}" ]]; then
			printf '%s\n' "${f#${ROOT}/}" >> "${SCAN_MISSING}"
			continue
		fi
		printf '%s\n' "${f#${ROOT}/}" >> "${SCAN_OPENED}"
		# CASE-INSENSITIVE. On macOS's case-insensitive filesystem
		# `#include "dsp/vcocore.hpp"` resolves perfectly well but did not match
		# the case-sensitive token, so the leak it represents was undetectable on
		# the primary development machine. No LFO-side include contains "vco" or
		# "morphblep" in any casing, so this adds no false positives.
		grep -niE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"][^\">]*${VCO_TOKEN}[^\">]*[>\"]" "${f}" \
			| sed "s|^|${f#${ROOT}/}:|" || true
	done
}

# ---------------------------------------------------------------------------
# THE RACK-FREE DETECTOR — [2/7]'s half of the boundary.
#
# Takes a list of files and echoes one "path:line:text" record per include
# directive whose target names a Rack SDK header. Empty output means clean.
#
# ONE PATH IS EXEMPT, AND ONLY ONE: the quoted path "dsp/RackCompat.hpp".
#
#   * It is this repository's own RACK-FREE compatibility shim, not a Rack SDK
#     header. It contains zero Rack includes and its bytes are pinned by
#     check_frozen.sh, so it cannot quietly acquire one.
#   * It is one of the four D-05 frozen shared headers the VCO is explicitly
#     ALLOWED to consume — check_canary.sh [5b/5] already allow-lists it by
#     name (DriftEngine.hpp | MathConst.hpp | RackCompat.hpp | Waveshape.hpp).
#     Without this exemption the two guards contradict each other about the
#     same include line, which is the state this repository was in before.
#   * Its FILENAME merely contains a substring this detector cannot tell apart
#     from a genuine SDK include. That is the whole of the false positive.
#   * D-14 MANDATES forge::exp2_taylor5 for V/oct → Hz, and that function lives
#     in exactly that shim. Without the exemption this section is a guaranteed
#     false positive the moment src/dsp/VcoCore.hpp includes what it uses —
#     `make guards` exits 1 and the CI toolchain-gate job turns red on the
#     first commit of the phase. Reproduced, not predicted.
#
# EXACT PATH, deliberately — not the basename and not a substring. A vendored
# "dsp/rack.hpp", a "dsp/RackSDK.hpp", a "dsp/RackCompatX.hpp" and any
# <rack...> angle-bracket SDK include all still FAIL. Widening this to a
# basename or substring match would reopen the entire hole the section exists
# to close.
#
# A function rather than an inline grep, for the same reason
# detect_vco_includes is one: [6/7] runs THIS function over two synthetic
# fixtures — one carrying a real SDK include, one carrying only the exempted
# shim — so the exemption is validated in both directions on every invocation
# rather than assumed. A control that re-implements the check it is validating
# proves only that two greps agree.
# ---------------------------------------------------------------------------
detect_rack_sdk_includes() {
	local f
	for f in "$@"; do
		[[ -f "${f}" ]] || continue
		grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^">]*[Rr]ack[^">]*[>"]' "${f}" \
			| grep -vE '#[[:space:]]*include[[:space:]]*"dsp/RackCompat\.hpp"' \
			| sed "s|^|${f#${ROOT}/}:|" || true
	done
}

# ---------------------------------------------------------------------------
# TRANSITIVE INCLUDE RESOLUTION.
#
# The detector above only sees DIRECT include lines, so ONE intermediate hop
# defeats it: an LFO-side file includes a helper, the helper includes a VCO
# header, and nothing in the scanned file names a VCO token. That bypass was
# built and confirmed green against the previous version of this gate.
#
# resolve_quoted_include maps a quoted include target to a real path the same
# way the compiler does — the INCLUDING FILE'S OWN DIRECTORY first, then the
# -Isrc / -Itests search paths the Makefile passes. expand_include_closure walks
# the resulting graph with a worklist and a seen set and echoes every file
# reachable from the given roots, roots included, each exactly once.
#
# Everything here is newline-delimited on purpose: this repository is developed
# under a directory whose name contains a space, so a space-delimited seen set
# or an unquoted path expansion would be silently and invisibly wrong.
# ---------------------------------------------------------------------------
resolve_quoted_include() {
	local includer="$1" target="$2" cand d
	d="$(dirname "${includer}")"
	for cand in "${d}/${target}" "${ROOT}/src/${target}" "${ROOT}/tests/${target}" "${ROOT}/${target}"; do
		if [[ -f "${cand}" ]]; then
			printf '%s\n' "${cand}"
			return 0
		fi
	done
	# Unresolvable (a system header, or a path this script cannot see). Not an
	# error: [3/7] is what constrains which includes a VCO header may carry.
	return 0
}

CLOSURE_SEEN="${TMP}/include_closure_seen.txt"
expand_include_closure() {
	local work=("$@")
	local idx=0 cur tgt res
	: > "${CLOSURE_SEEN}"
	while [[ "${idx}" -lt "${#work[@]}" ]]; do
		cur="${work[${idx}]}"
		idx=$((idx + 1))
		[[ -f "${cur}" ]] || continue
		if grep -Fxq -- "${cur}" "${CLOSURE_SEEN}"; then
			continue
		fi
		printf '%s\n' "${cur}" >> "${CLOSURE_SEEN}"
		printf '%s\n' "${cur}"
		while IFS= read -r tgt; do
			[[ -n "${tgt}" ]] || continue
			res="$(resolve_quoted_include "${cur}" "${tgt}")"
			if [[ -n "${res}" ]]; then
				work+=("${res}")
			fi
		done < <(sed -n 's/^[[:space:]]*#[[:space:]]*include[[:space:]]*"\([^"]*\)".*/\1/p' "${cur}" || true)
	done
}

# Collect the VCO header set once — used by [2/7] and [3/7].
VCO_HEADERS=()
for h in "${ROOT}"/src/dsp/Vco*.hpp; do
	[[ -f "${h}" ]] && VCO_HEADERS+=("${h}")
done
# Phase 32's morph-BLEP header joins the set the moment it lands.
[[ -f "${ROOT}/src/dsp/MorphBlep.hpp" ]] && VCO_HEADERS+=("${ROOT}/src/dsp/MorphBlep.hpp")

# ---------------------------------------------------------------------------
# [1/7] No LFO translation unit includes a VCO file, directly or transitively.
#
# A DENYLIST, NOT AN ALLOWLIST — this is load-bearing and was changed for cause.
#
# This section used to grep a hand-written 25-file list for DIRECT include lines.
# That scope was narrower than the contract in two independent ways, and both
# bypasses were built and confirmed fully green:
#
#   (a) A new file is not in the list, so it is not scanned. A new header under
#       src/dsp/ that includes dsp/VcoCore.hpp, pulled into src/plugin.cpp (which
#       is deliberately NOT pinned by check_frozen.sh), put VCO code in the
#       shipped plugin's registration translation unit with every gate green.
#   (b) Only DIRECT includes were matched, so one intermediate hop was invisible
#       even for a file that WAS in the list.
#
# The list is therefore derived, not written: everything under src/, tests/ and
# tools/ is LFO-side BY DEFAULT, and only the explicitly-named VCO-side files
# below are exempt. A file added by any future phase is covered the moment it
# lands rather than when someone remembers to add it here. The closure walk then
# closes (b).
#
# The VCO-side exemptions, each for a stated reason:
#   * src/vco_compile_canary.cpp — a src/ file that includes dsp/VcoCore.hpp on
#     purpose (D-07). That is the sanctioned direction, not a violation.
#   * src/AnalogVCO.cpp — Phase 30's VCO shell. When it lands it is VCO code and
#     belongs on the other side of this boundary.
#   * tests/VcoBlockDriver.hpp, tests/test_vco_harness.cpp — Phase 29's VCO
#     harness; scanning them would flag the gate into uselessness on its first run.
#   * tests/test_vco_core.cpp — Phase 30's CORE-01/CORE-03 behavioral suite
#     (plans 30-03 / 30-04). Same reason as the harness suite above, and it is
#     the same KIND of file: a VCO-side test TU whose entire purpose is to drive
#     forge::VcoCore through tests/VcoBlockDriver.hpp. It is not an LFO
#     translation unit, it links into no LFO build graph, and it cannot exist
#     without the include this section would otherwise flag. Adding it changes
#     the LFO-side scan set by exactly one file, and that file is VCO code.
#   * tests/test_vco_pitch.cpp — Phase 31's TEST-02 pitch / tuning /
#     exponential-FM suite (plans 31-05 / 31-06 / 31-07), added here as an
#     explicit, reviewable edit because 31-CONTEXT.md decision D-23 requires it
#     to be one. It is the same KIND of file as the two entries above it: a
#     VCO-side test translation unit whose entire purpose is to drive
#     forge::VcoCore through tests/VcoBlockDriver.hpp, which is an include this
#     section flags BY CONSTRUCTION. It is not an LFO translation unit and it
#     links into no LFO build graph.
#     The entry is added BEFORE the file exists, deliberately. That is the
#     Phase-29 precedent: src/AnalogVCO.cpp was pre-registered here while it was
#     still unwritten, and `make guards` was green on that file's very first run.
#     The entry immediately above was instead added reactively, after Phase 30's
#     TU had already landed and turned this section red — a recurring landmine
#     rather than a surprise, and pre-registration is what disarms it.
#     Adding it changes the LFO-side scan set by exactly one file, and that file
#     is VCO code. The match below compares against a QUOTED right-hand side, so
#     it is exact-path: no glob, no substring, no basename, and no sibling path
#     is exempted along with it.
# src/dsp/Vco*.hpp and src/dsp/MorphBlep.hpp are the VCO's own headers — the
# other side of the boundary, not scan targets.
#
# ADDING TO THE EXEMPTION LIST IS THE DANGEROUS EDIT HERE. Removing a file from
# the scan is now the only way to unguard it, which is exactly the property the
# old allowlist did not have.
# ---------------------------------------------------------------------------
echo "[1/7] No LFO translation unit includes a VCO file (directly or transitively)..."
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
	"tests/test_vco_core.cpp"
	"tests/test_vco_pitch.cpp"
)
LFO_SCAN=()
while IFS= read -r f; do
	rel="${f#${ROOT}/}"
	case "${rel}" in
		src/dsp/Vco*.hpp|src/dsp/MorphBlep.hpp) continue ;;
	esac
	skip=0
	for a in "${VCO_SIDE_ALLOW[@]}"; do
		if [[ "${rel}" == "${a}" ]]; then skip=1; fi
	done
	if [[ "${skip}" -eq 0 ]]; then
		LFO_SCAN+=("${f}")
	fi
done < <(find "${ROOT}/src" "${ROOT}/tests" "${ROOT}/tools" -type f \
	\( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) | sort)

if [[ "${#LFO_SCAN[@]}" -eq 0 ]]; then
	note_fail "derived ZERO LFO-side files from src/, tests/ and tools/. The find above returned nothing, so this section is scanning an empty set and its PASS would mean nothing."
else
	# Follow one level of indirection and then some: the closure covers a chain of
	# any depth, so no number of intermediate hops hides the include.
	LFO_CLOSURE=()
	while IFS= read -r f; do
		[[ -n "${f}" ]] && LFO_CLOSURE+=("${f}")
	done < <(expand_include_closure "${LFO_SCAN[@]}")

	: > "${SCAN_OPENED}"
	: > "${SCAN_MISSING}"
	lfo_hits="$(detect_vco_includes "${LFO_CLOSURE[@]}")"
	# The REAL number of files opened, not the array length. These can only differ
	# if a path was handed to the detector and did not exist, which is reported
	# immediately below rather than skipped in silence.
	scanned="$(grep -c . "${SCAN_OPENED}" || true)"
	if [[ -s "${SCAN_MISSING}" ]]; then
		note_fail "the scan set names file(s) that are not on disk, so this section covers LESS than it reports:"
		sed 's/^/    /' "${SCAN_MISSING}"
	fi
	if [[ -n "${lfo_hits}" ]]; then
		note_fail "VCO header(s) reached the LFO build graph — this changes what the SHIPPED module compiles to:"
		echo "${lfo_hits}" | sed 's/^/    /'
		echo "        A hit on a file that is not itself LFO code means an LFO-side file"
		echo "        reaches it through an include chain. Break the chain, or move the"
		echo "        file to the VCO side of the boundary in VCO_SIDE_ALLOW above."
	else
		echo "  OK: ${#LFO_SCAN[@]} LFO-side root file(s), ${scanned} file(s) opened across their transitive include closure, zero VCO includes"
	fi
fi

# ---------------------------------------------------------------------------
# [2/7] VCO headers are Rack-free.
#
# Also enforced by construction — the Makefile `test` target passes no
# -I$(RACK_DIR)/include, so a Rack include in a VCO header would fail to
# compile there. This section catches the case where someone ADDS a Rack path
# (to the test target, to a new target, or by vendoring a header), which would
# make the breach compile silently.
#
# The detection itself lives in detect_rack_sdk_includes above, together with
# the one documented exact-path exemption ("dsp/RackCompat.hpp", the repo's own
# Rack-FREE shim) and the reasoning for it. [6/7] runs that same function over
# two synthetic fixtures, so the exemption is proven both to still catch a
# genuine SDK include and to not flag the shim, every time this script runs.
# ---------------------------------------------------------------------------
echo "[2/7] VCO headers are Rack-free..."
if [[ "${#VCO_HEADERS[@]}" -eq 0 ]]; then
	note_fail "no src/dsp/Vco*.hpp found at all — the VCO seam has vanished"
else
	for h in "${VCO_HEADERS[@]}"; do
		rel="${h#${ROOT}/}"
		rack_hits="$(detect_rack_sdk_includes "${h}")"
		if [[ -n "${rack_hits}" ]]; then
			note_fail "${rel} includes a Rack SDK header — VCO DSP must stay Rack-free (TEST-02):"
			echo "${rack_hits}" | sed 's/^/    /'
		else
			echo "  OK: ${rel} — no Rack include"
		fi
	done
fi

# ---------------------------------------------------------------------------
# [3/7] VCO headers include only dsp/ siblings and standard headers.
#
# Every include must be either <angle-bracket> (standard library) or a quoted
# path beginning with dsp/. Anything else — a tests/ header, a relative ../
# escape, a vendored third-party path — is a dependency this seam must not have.
# ---------------------------------------------------------------------------
echo "[3/7] VCO headers include only dsp/ siblings and standard headers..."
if [[ "${#VCO_HEADERS[@]}" -gt 0 ]]; then
	for h in "${VCO_HEADERS[@]}"; do
		rel="${h#${ROOT}/}"
		bad=""
		while IFS= read -r line; do
			[[ -z "${line}" ]] && continue
			# Angle-bracket include -> standard library, always fine.
			if printf '%s\n' "${line}" | grep -qE '#[[:space:]]*include[[:space:]]*<'; then
				continue
			fi
			# Quoted include -> must start with dsp/.
			if printf '%s\n' "${line}" | grep -qE '#[[:space:]]*include[[:space:]]*"dsp/'; then
				continue
			fi
			bad="${bad}${line}"$'\n'
		done < <(grep -nE '^[[:space:]]*#[[:space:]]*include' "${h}" || true)
		if [[ -n "${bad}" ]]; then
			note_fail "${rel} has include(s) that are neither a standard header nor a dsp/ sibling:"
			printf '%s' "${bad}" | sed 's/^/    /'
		else
			echo "  OK: ${rel} — siblings and standard headers only"
		fi
	done
fi

# ---------------------------------------------------------------------------
# [4/7] R-9 ODR guard — exactly one forge::Inputs, and it is LfoCore.hpp's.
#
# Why this is not paranoia: two definitions of `forge::Inputs` with different
# members are an ODR violation across translation units. Each TU that sees only
# one of them compiles without a diagnostic; the linker merges them without a
# diagnostic on Apple clang; the program then has undefined behavior in the
# field. This is structurally the SAME failure class that got v2.0.0 rejected —
# a construct that is locally clean and globally wrong. The VCO's input POD is
# named forge::VcoInputs precisely so this can never happen by accident.
# ---------------------------------------------------------------------------
echo "[4/7] R-9 ODR guard — exactly one forge::Inputs..."
# The brace must NOT be required on the same line. The previous regex ended in
# [{:], so Allman style —
#
#     struct Inputs
#     {
#         float x = 0.f;
#     };
#
# — did not match, and a second forge::Inputs (the exact R-9 trap this section is
# dedicated to) sailed through. Allman is common enough that this was not
# hypothetical. Verified evasion before the change.
#
# The trailing group is what keeps this precise while dropping that requirement:
# after `struct Inputs` the line must end (Allman), or continue with `{` or `:`.
#   struct Inputs {        -> match      struct Inputs : Base {  -> match
#   struct Inputs          -> match      struct Inputs;          -> NO match (fwd decl)
#   struct Inputs in;      -> NO match   struct InputsFoo {      -> NO match
# Forward declarations therefore fall out for free — `;` is neither `{`, `:` nor
# end of line — with no second grep to keep in sync.
inputs_decls="$(grep -rnE '^[[:space:]]*struct[[:space:]]+Inputs[[:space:]]*([{:].*)?$' \
	--include='*.hpp' --include='*.h' --include='*.cpp' "${ROOT}/src" || true)"
inputs_files="$(printf '%s\n' "${inputs_decls}" | grep -v '^$' | cut -d: -f1 | sort -u || true)"
inputs_count="$(printf '%s\n' "${inputs_files}" | grep -c . || true)"
if [[ "${inputs_count}" -eq 1 ]] && [[ "${inputs_files}" == "${ROOT}/src/dsp/LfoCore.hpp" ]]; then
	echo "  OK: exactly one 'struct Inputs' under src/, in src/dsp/LfoCore.hpp"
elif [[ "${inputs_count}" -eq 0 ]]; then
	note_fail "no 'struct Inputs' found under src/ — src/dsp/LfoCore.hpp's POD core boundary has moved or been renamed; this guard no longer guards anything"
else
	note_fail "expected exactly one 'struct Inputs' under src/ (in src/dsp/LfoCore.hpp), found ${inputs_count}:"
	printf '%s\n' "${inputs_decls}" | sed "s|${ROOT}/||" | sed 's/^/    /'
	echo "        A second forge::Inputs compiles cleanly in any translation unit that"
	echo "        includes only one of them and links without a diagnostic, but it is a"
	echo "        cross-translation-unit ODR violation with UNDEFINED BEHAVIOR. Name the"
	echo "        VCO's POD forge::VcoInputs (as src/dsp/VcoCore.hpp already does)."
fi

# ---------------------------------------------------------------------------
# [5/7] Placement rule — no hashing IMPLEMENTATION under src/.
#
# The SHA-256 in this milestone is an integrity tripwire for frozen sources and
# golden fixtures. It is NOT a security control: nothing about it resists an
# adversary who can also edit the manifest. It lives in tests/Sha256.hpp and must
# never enter the shipped C++11 build graph.
#
# Scope note (a deliberate refinement of the rule as written): the FILENAME check
# covers C/C++ SOURCE files only. src/dsp/FROZEN.sha256 is this milestone's
# manifest — a data file, named for the algorithm it records digests in, with no
# implementation in it — and lives beside the headers it pins on purpose. Gating
# on the bare substring would fail on the very artifact the rule exists to
# support. The constant check below is unrestricted and would still catch an
# implementation smuggled in under any filename.
# ---------------------------------------------------------------------------
echo "[5/7] No hashing implementation under src/..."
hash_named="$(find "${ROOT}/src" -type f \
	\( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cc' -o -name '*.cxx' \) \
	-name '*ha256*' 2>/dev/null || true)"
if [[ -n "${hash_named}" ]]; then
	note_fail "hashing source file(s) under src/ — the tripwire hasher belongs to tests/ scope only:"
	printf '%s\n' "${hash_named}" | sed "s|${ROOT}/||" | sed 's/^/    /'
else
	echo "  OK: no SHA-256 source file under src/"
fi
# The SHA-256 initial hash state. Its presence is unambiguous evidence of an
# implementation, whatever the file is called.
#
# CASE-INSENSITIVE, deliberately. The comment above claims this check "is
# unrestricted and would still catch an implementation smuggled in under any
# filename", but `grep '0x6a09e667'` does not match `0x6A09E667u`, which is how a
# large fraction of published SHA-256 implementations write it — so the claim was
# false for the most common spelling. Verified.
#
# Two constants rather than one: H0 alone can be omitted or computed, while a
# partial table is still a hash implementation. 0xbb67ae85 is H1.
const_hits="$(grep -rniE '0x6a09e667|0xbb67ae85' "${ROOT}/src" || true)"
if [[ -n "${const_hits}" ]]; then
	note_fail "SHA-256 initial-state constant found under src/ — a hash implementation has entered the shipped build graph:"
	printf '%s\n' "${const_hits}" | sed "s|${ROOT}/||" | sed 's/^/    /'
else
	echo "  OK: no SHA-256 initial-state constant under src/"
fi

# ---------------------------------------------------------------------------
# [6/7] NEGATIVE CONTROLS — the detectors must report real violations.
#
# A guard that has only ever been observed green is unvalidated. This section
# validates TWO detectors, each by running the EXACT function its section runs
# over a synthetic fixture:
#
#   * detect_vco_includes, which [1/7] depends on — a DIRECT (one-hop) fixture
#     and a TWO-HOP fixture, both of which must produce a hit.
#   * detect_rack_sdk_includes, which [2/7] depends on — a fixture carrying a
#     genuine Rack SDK include, which must produce a hit, AND a fixture
#     carrying only the exempted shim path, which must NOT. Both directions
#     are required: the first alone would still pass if the exemption were
#     widened to a substring match, and the second alone would still pass if
#     the detector were deleted outright. Together they pin the exemption to
#     exactly the width it is documented to have.
#
# If any of these fixtures behaves the wrong way, the corresponding section's
# green above means nothing and this section fails the gate.
# ---------------------------------------------------------------------------
echo "[6/7] NEGATIVE CONTROLS — the VCO-leak and Rack-free detectors must behave on synthetic fixtures..."
cat > "${TMP}/nc_lfo_leak.cpp" <<'EOF'
// Synthetic fixture: an LFO-side translation unit that pulls in VCO code.
// This is exactly the breach [1/7] exists to prevent.
#include "dsp/LfoCore.hpp"
#include "dsp/VcoCore.hpp"
float ncLeakProbe() { return 0.f; }
EOF
nc_hits="$(detect_vco_includes "${TMP}/nc_lfo_leak.cpp")"
if [[ -n "${nc_hits}" ]]; then
	echo "  OK: direct (one-hop) violation detected by the same detector [1/7] uses:"
	printf '%s\n' "${nc_hits}" | sed 's/^/    /'
else
	note_fail "negative control DID NOT FIRE: detect_vco_includes reported a fixture that plainly includes dsp/VcoCore.hpp as clean. The detector is broken, so [1/7]'s PASS above is meaningless."
fi

# TWO-HOP control. The one-hop fixture above uses a DIRECT #include, which is the
# one shape the raw detector always handled — on its own it validates the regex,
# not the scope. This fixture uses the shape the real bypass used: an LFO-side
# translation unit that names no VCO token anywhere in its own text, reaching a
# VCO header through an intermediate header. It exercises expand_include_closure
# AND the detector together, which is the combination [1/7] actually depends on.
mkdir -p "${TMP}/nc2/dsp"
cat > "${TMP}/nc2/dsp/NcIntermediate.hpp" <<'EOF'
#pragma once
// The intermediate hop.
#include "dsp/VcoCore.hpp"
EOF
cat > "${TMP}/nc2/nc_two_hop.cpp" <<'EOF'
// Synthetic fixture: an LFO-side translation unit that reaches VCO code through
// ONE intermediate header. Grepping this file's own text finds nothing.
#include "dsp/LfoCore.hpp"
#include "dsp/NcIntermediate.hpp"
float ncTwoHopProbe() { return 0.f; }
EOF
nc2_files=()
while IFS= read -r ncf; do
	[[ -n "${ncf}" ]] && nc2_files+=("${ncf}")
done < <(expand_include_closure "${TMP}/nc2/nc_two_hop.cpp")
nc2_direct="$(detect_vco_includes "${TMP}/nc2/nc_two_hop.cpp")"
nc2_hits="$(detect_vco_includes "${nc2_files[@]}")"
if [[ -n "${nc2_direct}" ]]; then
	note_fail "two-hop negative control is INVALID: the fixture's own text matches the detector, so it is not testing indirection at all. Rewrite nc_two_hop.cpp so it names no VCO token."
elif [[ -n "${nc2_hits}" ]]; then
	echo "  OK: TWO-HOP violation detected through the same include closure [1/7] uses:"
	printf '%s\n' "${nc2_hits}" | sed "s|${TMP}/|<scratch>/|" | sed 's/^/    /'
else
	note_fail "two-hop negative control DID NOT FIRE: a fixture that reaches dsp/VcoCore.hpp through ONE intermediate header was reported clean. expand_include_closure is not resolving includes, so [1/7] has silently reverted to catching DIRECT includes only — the exact bypass it was rewritten to close."
fi

# EXEMPTION CONTROLS for [2/7], in BOTH directions.
#
# [2/7] carries a documented exact-path exemption for "dsp/RackCompat.hpp". An
# exemption nobody exercises is indistinguishable from a hole: widen it by one
# character and the section keeps reporting PASS on a VCO header that really
# does pull in the Rack SDK. These two fixtures run through
# detect_rack_sdk_includes — the SAME function [2/7] calls, for the same reason
# the controls above call the same function [1/7] calls. A control that
# exercises different code than the guard proves nothing.
cat > "${TMP}/VcoNcRackSdkProbe.hpp" <<'EOF'
#pragma once
// Synthetic fixture: a VCO-shaped header carrying a GENUINE Rack SDK include.
// This is exactly the breach [2/7] exists to prevent.
#include <rack.hpp>
EOF
nc3_hits="$(detect_rack_sdk_includes "${TMP}/VcoNcRackSdkProbe.hpp")"
if [[ -n "${nc3_hits}" ]]; then
	echo "  OK: [2/7] detector still reports a genuine Rack SDK include:"
	printf '%s\n' "${nc3_hits}" | sed "s|${TMP}/|<scratch>/|" | sed 's/^/    /'
else
	note_fail "the Rack-free negative control DID NOT FIRE: detect_rack_sdk_includes reported a fixture that plainly includes <rack.hpp> as clean. The RackCompat exemption has swallowed the whole detector, so [2/7]'s PASS above is meaningless."
fi

cat > "${TMP}/VcoNcRackShimProbe.hpp" <<'EOF'
#pragma once
// Synthetic fixture: a VCO-shaped header carrying ONLY the exempted path —
// the repo's own Rack-FREE shim, which D-14 makes mandatory for the VCO.
#include "dsp/RackCompat.hpp"
EOF
nc4_hits="$(detect_rack_sdk_includes "${TMP}/VcoNcRackShimProbe.hpp")"
if [[ -z "${nc4_hits}" ]]; then
	echo "  OK: [2/7] detector ignores the one exempted path, dsp/RackCompat.hpp"
else
	note_fail "the documented exemption is NOT in effect: detect_rack_sdk_includes flagged a fixture whose only include is the exempted \"dsp/RackCompat.hpp\". D-14's mandated forge::exp2_taylor5 lives in that shim, so every VCO header that includes what it uses will hard-fail this build gate:"
	printf '%s\n' "${nc4_hits}" | sed "s|${TMP}/|<scratch>/|" | sed 's/^/    /'
fi

echo "  NOTE: these negative controls are what make this audit VALIDATED rather than"
echo "        merely green. They run on every invocation. Do not remove them."

# ---------------------------------------------------------------------------
# [7/7] Guard wiring (P-5) — every guard script must be invoked by CI.
#
# This repository has already proven the failure mode. tests/check_docs.sh is a
# complete, correct, passing gate that has been referenced by NOTHING since it
# was written in Phase 27 — not the Makefile, not the workflow. It has provided
# exactly zero assurance for its entire existence while appearing, to anyone
# reading the tests directory, to be a guard. An unwired guard is worse than no
# guard: it is a guard plus a false belief.
#
# So the wiring itself is now a gate. Every tests/check_*.sh must be named in
# .github/workflows/test.yml, or be on the exemption list below with a reason.
# A future guard script that is neither wired nor exempted fails this audit.
# ---------------------------------------------------------------------------
echo "[7/7] Guard wiring (P-5) — every guard script is invoked by CI..."
WORKFLOW_REL=".github/workflows/test.yml"
WORKFLOW="${ROOT}/${WORKFLOW_REL}"

# Documented exemptions. Each entry needs a reason, in a comment, right here.
#
#   check_docs.sh — a Phase 27 user-manual documentation gate (brand denylist,
#     section-file existence, code-fact tokens). It passes today but is invoked
#     by nothing. Wiring it is a one-line CI step and is deliberately OUT OF
#     SCOPE for Phase 29, whose subject is the LFO non-regression guardrail, not
#     the manual. It is tracked as a real, visible task in
#     .planning/todos/pending/wire-check-docs-into-ci.md rather than being
#     silently absorbed by this exemption list.
GUARD_WIRING_EXEMPT=(
	"check_docs.sh"
)

if [[ ! -f "${WORKFLOW}" ]]; then
	note_fail "missing ${WORKFLOW_REL} — every guard in this repository is now unwired"
else
	# MENTIONING IS NOT RUNNING — which is the entire premise of this section.
	#
	# The check used to be `grep -qF -- "${grel}" "${WORKFLOW}"`, which matches
	# anywhere in the file: a YAML comment, a `name:` string, a changed-paths
	# filter. A script named in a comment but never invoked — precisely the
	# check_docs.sh situation this section was written to prevent — reported
	# "OK: ... wired into CI". The section's implementation only checked the very
	# thing its premise says is insufficient.
	#
	# So: strip comment lines and `name:` strings, then require the path to appear
	# in an EXECUTABLE position — as the argument of bash/sh, or invoked directly.
	WORKFLOW_EXEC="${TMP}/workflow_exec.txt"
	grep -v '^[[:space:]]*#' "${WORKFLOW}" | grep -vE '^[[:space:]]*(-[[:space:]]+)?name:' > "${WORKFLOW_EXEC}" || true

	# THE SAME RULE, ONE LEVEL UP. `make guards` and this workflow were two
	# independent hardcoded lists of the same scripts, and CI ran only the
	# scripts — so the Makefile list, which the Makefile banner advertises as "one
	# local command for every standing guard this milestone adds", had zero
	# execution coverage. A guard wired into CI but forgotten in GUARD_SCRIPTS
	# left it silently incomplete: P-5 again, one level up. CI now runs `make
	# guards` too, and every guard must appear in BOTH lists, so they cannot
	# drift apart in either direction.
	MAKEFILE_REL="Makefile"
	MAKEFILE="${ROOT}/${MAKEFILE_REL}"
	make_guards_in_ci=0
	if grep -qE '(^|[^[:alnum:]_./-])make[[:space:]]+guards([[:space:]]|;|&|\||$)' "${WORKFLOW_EXEC}"; then
		make_guards_in_ci=1
	fi
	# Collect GUARD_SCRIPTS, following backslash line-continuations.
	guard_list=""
	if [[ -f "${MAKEFILE}" ]]; then
		guard_list="$(awk '
			/^GUARD_SCRIPTS[[:space:]]*:?=/ { collecting = 1 }
			collecting {
				line = $0
				sub(/^GUARD_SCRIPTS[[:space:]]*:?=/, "", line)
				sub(/\\$/, "", line)
				printf "%s ", line
				if ($0 !~ /\\$/) collecting = 0
			}
		' "${MAKEFILE}")"
	fi
	if [[ ! -f "${MAKEFILE}" ]]; then
		note_fail "missing ${MAKEFILE_REL} — \`make guards\` cannot exist, so every guard below has one fewer way of being run"
	elif [[ -z "${guard_list// /}" ]]; then
		note_fail "no GUARD_SCRIPTS assignment found in ${MAKEFILE_REL} — \`make guards\` runs nothing, and this section can no longer tell whether it does"
	elif [[ "${make_guards_in_ci}" -eq 0 ]]; then
		note_fail "${WORKFLOW_REL} never runs \`make guards\`, so the Makefile's GUARD_SCRIPTS list has NO execution coverage. A guard omitted from it would leave the advertised one-command local guard suite silently incomplete."
	else
		echo "  OK: ${WORKFLOW_REL} runs \`make guards\`, so GUARD_SCRIPTS is executed by CI"
	fi

	for g in "${ROOT}"/tests/check_*.sh; do
		[[ -f "${g}" ]] || continue
		gbase="$(basename "${g}")"
		grel="tests/${gbase}"
		grel_re="${grel//./\\.}"
		exempt=0
		for e in "${GUARD_WIRING_EXEMPT[@]}"; do
			if [[ "${gbase}" == "${e}" ]]; then exempt=1; fi
		done
		# In GUARD_SCRIPTS? A guard missing from it is skipped by `make guards`.
		in_make_list=0
		case " ${guard_list} " in
			*" ${grel} "*) in_make_list=1 ;;
		esac

		wired_workflow=0
		if grep -qE "((bash|sh)[[:space:]]+|\./)${grel_re}([[:space:]]|;|&|\||$)" "${WORKFLOW_EXEC}"; then
			wired_workflow=1
		fi

		# Running via `make guards` is real CI wiring too, so a guard reached only
		# that way is not reported as unwired.
		wired=0
		if [[ "${wired_workflow}" -eq 1 ]]; then
			wired=1
		elif [[ "${in_make_list}" -eq 1 ]] && [[ "${make_guards_in_ci}" -eq 1 ]]; then
			wired=1
		fi

		if [[ "${wired}" -eq 1 ]] && [[ "${in_make_list}" -eq 0 ]]; then
			note_fail "${grel} is invoked by ${WORKFLOW_REL} but is NOT in the Makefile GUARD_SCRIPTS list, so \`make guards\` silently skips it. The command the Makefile advertises as covering every standing guard would then be incomplete, and a developer running it locally would get a green that CI would not."
		fi

		if [[ "${wired}" -eq 1 ]]; then
			if [[ "${exempt}" -eq 1 ]]; then
				echo "  OK: ${grel} — wired into CI (it may be removed from the exemption list)"
			else
				echo "  OK: ${grel} — wired into CI"
			fi
		elif [[ "${exempt}" -eq 1 ]]; then
			echo "  EXEMPT: ${grel} — documented exemption, not wired (see the comment above and .planning/todos/pending/wire-check-docs-into-ci.md)"
		else
			note_fail "${grel} is a guard script that NOTHING invokes. It is not INVOKED by ${WORKFLOW_REL} (being named in a comment or a step title does not count — mentioning is not running) and it is not on the documented exemption list. Add a CI step for it in the same commit that created it (P-5), or add it to GUARD_WIRING_EXEMPT with a written reason."
		fi
	done
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${fail}" -eq 0 ]]; then
	echo "PASS: dependency-direction audit clean (D-06 + R-9 ODR + hasher placement + negative control + guard wiring)."
	exit 0
else
	echo "FAIL: dependency-direction audit found problems (see above)."
	exit 1
fi
