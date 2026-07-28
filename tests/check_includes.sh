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
#   [1/6] no LFO translation unit includes a VCO file
#   [2/6] VCO headers are Rack-free
#   [3/6] VCO headers include only dsp/ siblings and standard headers
#   [4/6] R-9 ODR guard — exactly one forge::Inputs, in src/dsp/LfoCore.hpp
#   [5/6] no hashing implementation under src/
#   [6/6] NEGATIVE CONTROL — the section-1 detector demonstrably reports a
#         synthetic violation, so this audit is validated rather than merely
#         green. It runs the SAME function section 1 runs: a control that
#         exercises different code than the guard proves nothing.
#
# Needs no compiler and no Rack SDK, runs from any working directory, and leaves
# no artifacts. Returns 0 (PASS) only when every group passes.

set -euo pipefail

# Resolve repo root relative to this script so it runs from anywhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }

# Scratch dir for the synthetic fixture in [6/6]; removed on exit so a run never
# leaves residue in the working tree.
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

# ---------------------------------------------------------------------------
# THE DETECTOR.
#
# Takes a list of files and echoes one "path:line:text" record per include
# directive whose target names a VCO file. Empty output means clean.
#
# This is deliberately a function rather than an inline grep: [6/6] runs THIS
# function over a synthetic violating fixture, so the negative control validates
# the exact code path [1/6] depends on. A control that re-implements the check it
# is validating proves only that two greps agree.
# ---------------------------------------------------------------------------
VCO_TOKEN='(Vco|MorphBlep)'
detect_vco_includes() {
	local f
	for f in "$@"; do
		[[ -f "${f}" ]] || continue
		grep -nE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"][^\">]*${VCO_TOKEN}[^\">]*[>\"]" "${f}" \
			| sed "s|^|${f#${ROOT}/}:|" || true
	done
}

# Collect the VCO header set once — used by [2/6] and [3/6].
VCO_HEADERS=()
for h in "${ROOT}"/src/dsp/Vco*.hpp; do
	[[ -f "${h}" ]] && VCO_HEADERS+=("${h}")
done
# Phase 32's morph-BLEP header joins the set the moment it lands.
[[ -f "${ROOT}/src/dsp/MorphBlep.hpp" ]] && VCO_HEADERS+=("${ROOT}/src/dsp/MorphBlep.hpp")

# ---------------------------------------------------------------------------
# [1/6] No LFO translation unit includes a VCO file.
#
# An EXPLICIT allowlist, not a glob. The VCO's own files and this phase's new
# test files (tests/VcoBlockDriver.hpp, tests/test_vco_harness.cpp) legitimately
# include VCO headers and must not be scanned — a glob would flag them and the
# gate would be edited into uselessness on its first run.
#
# Also intentionally outside the scan set:
#   * src/vco_compile_canary.cpp — a src/ file that includes dsp/VcoCore.hpp on
#     purpose (D-07). That is the sanctioned direction, not a violation.
#   * src/AnalogVCO.cpp — Phase 30's VCO shell. When it lands it is VCO code and
#     belongs on the other side of this boundary.
#
# When a new LFO-side file is added, add it here. A file that is neither listed
# here nor VCO code is a file this gate does not cover, which is why the list is
# spelled out rather than derived.
# ---------------------------------------------------------------------------
echo "[1/6] No LFO translation unit includes a VCO file..."
LFO_SCAN=(
	# shipped shell + Rack registration
	"${ROOT}/src/AnalogLFO.cpp"
	"${ROOT}/src/plugin.cpp"
	"${ROOT}/src/plugin.hpp"
	# the eleven-header LFO include closure
	"${ROOT}/src/dsp/Anim.hpp"
	"${ROOT}/src/dsp/ClockTracker.hpp"
	"${ROOT}/src/dsp/DisplayFill.hpp"
	"${ROOT}/src/dsp/DriftEngine.hpp"
	"${ROOT}/src/dsp/LfoCore.hpp"
	"${ROOT}/src/dsp/MathConst.hpp"
	"${ROOT}/src/dsp/PatchParse.hpp"
	"${ROOT}/src/dsp/RackCompat.hpp"
	"${ROOT}/src/dsp/RatioTable.hpp"
	"${ROOT}/src/dsp/Swing.hpp"
	"${ROOT}/src/dsp/Waveshape.hpp"
	# the LFO test driver
	"${ROOT}/tests/BlockDriver.hpp"
	# the nine test translation units that predate Phase 29
	"${ROOT}/tests/test_anim.cpp"
	"${ROOT}/tests/test_display.cpp"
	"${ROOT}/tests/test_dsp_stateful.cpp"
	"${ROOT}/tests/test_dsp_units.cpp"
	"${ROOT}/tests/test_extraction.cpp"
	"${ROOT}/tests/test_golden.cpp"
	"${ROOT}/tests/test_invariants.cpp"
	"${ROOT}/tests/test_regression.cpp"
	"${ROOT}/tests/test_smoke.cpp"
	# the golden capture tool — its output IS the LFO's frozen behavior
	"${ROOT}/tools/capture_golden.cpp"
)
lfo_hits="$(detect_vco_includes "${LFO_SCAN[@]}")"
if [[ -n "${lfo_hits}" ]]; then
	note_fail "VCO header(s) reached the LFO build graph — this changes what the SHIPPED module compiles to:"
	echo "${lfo_hits}" | sed 's/^/    /'
else
	echo "  OK: ${#LFO_SCAN[@]} LFO-side files scanned, zero VCO includes"
fi

# ---------------------------------------------------------------------------
# [2/6] VCO headers are Rack-free.
#
# Also enforced by construction — the Makefile `test` target passes no
# -I$(RACK_DIR)/include, so a Rack include in a VCO header would fail to
# compile there. This section catches the case where someone ADDS a Rack path
# (to the test target, to a new target, or by vendoring a header), which would
# make the breach compile silently.
# ---------------------------------------------------------------------------
echo "[2/6] VCO headers are Rack-free..."
if [[ "${#VCO_HEADERS[@]}" -eq 0 ]]; then
	note_fail "no src/dsp/Vco*.hpp found at all — the VCO seam has vanished"
else
	for h in "${VCO_HEADERS[@]}"; do
		rel="${h#${ROOT}/}"
		rack_hits="$(grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^">]*[Rr]ack[^">]*[>"]' "${h}" || true)"
		if [[ -n "${rack_hits}" ]]; then
			note_fail "${rel} includes a Rack SDK header — VCO DSP must stay Rack-free (TEST-02):"
			echo "${rack_hits}" | sed 's/^/    /'
		else
			echo "  OK: ${rel} — no Rack include"
		fi
	done
fi

# ---------------------------------------------------------------------------
# [3/6] VCO headers include only dsp/ siblings and standard headers.
#
# Every include must be either <angle-bracket> (standard library) or a quoted
# path beginning with dsp/. Anything else — a tests/ header, a relative ../
# escape, a vendored third-party path — is a dependency this seam must not have.
# ---------------------------------------------------------------------------
echo "[3/6] VCO headers include only dsp/ siblings and standard headers..."
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
# [4/6] R-9 ODR guard — exactly one forge::Inputs, and it is LfoCore.hpp's.
#
# Why this is not paranoia: two definitions of `forge::Inputs` with different
# members are an ODR violation across translation units. Each TU that sees only
# one of them compiles without a diagnostic; the linker merges them without a
# diagnostic on Apple clang; the program then has undefined behavior in the
# field. This is structurally the SAME failure class that got v2.0.0 rejected —
# a construct that is locally clean and globally wrong. The VCO's input POD is
# named forge::VcoInputs precisely so this can never happen by accident.
# ---------------------------------------------------------------------------
echo "[4/6] R-9 ODR guard — exactly one forge::Inputs..."
inputs_decls="$(grep -rnE '^[[:space:]]*struct[[:space:]]+Inputs[[:space:]]*[{:]' \
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
# [5/6] Placement rule — no hashing IMPLEMENTATION under src/.
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
echo "[5/6] No hashing implementation under src/..."
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
const_hits="$(grep -rn '0x6a09e667' "${ROOT}/src" || true)"
if [[ -n "${const_hits}" ]]; then
	note_fail "SHA-256 initial-state constant found under src/ — a hash implementation has entered the shipped build graph:"
	printf '%s\n' "${const_hits}" | sed "s|${ROOT}/||" | sed 's/^/    /'
else
	echo "  OK: no SHA-256 initial-state constant under src/"
fi

# ---------------------------------------------------------------------------
# [6/6] NEGATIVE CONTROL — the detector must report a real violation.
#
# A guard that has only ever been observed green is unvalidated. This writes a
# synthetic LFO-side translation unit that includes a VCO header, runs the SAME
# detect_vco_includes function [1/6] runs, and REQUIRES a hit. If the detector
# reports the fixture clean, [1/6]'s green above means nothing and this section
# fails the gate.
# ---------------------------------------------------------------------------
echo "[6/6] NEGATIVE CONTROL — the audit must detect a synthetic violation..."
cat > "${TMP}/nc_lfo_leak.cpp" <<'EOF'
// Synthetic fixture: an LFO-side translation unit that pulls in VCO code.
// This is exactly the breach [1/6] exists to prevent.
#include "dsp/LfoCore.hpp"
#include "dsp/VcoCore.hpp"
float ncLeakProbe() { return 0.f; }
EOF
nc_hits="$(detect_vco_includes "${TMP}/nc_lfo_leak.cpp")"
if [[ -n "${nc_hits}" ]]; then
	echo "  OK: synthetic violation detected by the same detector [1/6] uses:"
	printf '%s\n' "${nc_hits}" | sed 's/^/    /'
else
	note_fail "negative control DID NOT FIRE: detect_vco_includes reported a fixture that plainly includes dsp/VcoCore.hpp as clean. The detector is broken, so [1/6]'s PASS above is meaningless."
fi
echo "  NOTE: this negative control is what makes this audit VALIDATED rather than"
echo "        merely green. It runs on every invocation. Do not remove it."

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${fail}" -eq 0 ]]; then
	echo "PASS: dependency-direction audit clean (D-06 + R-9 ODR + hasher placement + negative control)."
	exit 0
else
	echo "FAIL: dependency-direction audit found problems (see above)."
	exit 1
fi
