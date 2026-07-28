#!/usr/bin/env bash
#
# check_canary.sh — the standing guard on the D-07 / D-08 VCO compile canary.
#
# The canary (src/vco_compile_canary.cpp) is what makes `make strict` and the CI
# MinGW compile-plus-link leg actually cover VCO code. This script guards the
# canary itself, because a canary can rot in three distinct ways that all leave
# every gate reporting PASS:
#
#   (1) it is deleted, or its probe is renamed / made static
#   (2) it is reduced to a bare #include, so the translation unit emits no code,
#       odr-uses nothing, and gives the link leg nothing to resolve (Pitfall P-1)
#   (3) a later phase adds a VCO header and forgets to include it here, so that
#       header silently never reaches either gate (the D-08 growth rule)
#
# Enforces:
#   [1/5] the canary exists and both declares and defines its probe
#   [2/5] the canary EMITS a defined external-linkage probe symbol (P-1 guard)
#   [3/5] the canary compiles clean under -std=c++11 -pedantic-errors
#   [4/5] NEGATIVE CONTROL — the C++17-ism gate demonstrably REJECTS known-bad
#         synthetic translation units, so this guard is validated by an observed
#         red rather than by never having been anything but green
#   [5/5] D-08 growth rule — every src/dsp/Vco*.hpp is included by the canary
#
# Deliberately NOT checked here: that the ODR / in-class `static constexpr`
# failure class is caught. It cannot be reproduced locally — Apple clang
# materializes that construct as a per-translation-unit local symbol and links
# cleanly at every optimization level. That gate exists ONLY in the CI MinGW
# link leg, and asserting it here would be reporting a result this machine
# cannot produce.
#
# Needs no Rack SDK and runs from any working directory. Leaves no artifacts.
# Returns 0 (PASS) only when all five groups pass.

set -euo pipefail

# Resolve repo root relative to this script so it runs from anywhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# The canary's repo-root-relative path (Phase 29 plan 29-03 Task 1: `option-a`).
# If this ever moves, update it here AND in the Makefile/CI if the new location
# is outside the src/*.cpp globs.
CANARY_REL="src/vco_compile_canary.cpp"
CANARY="${ROOT}/${CANARY_REL}"

CXX_BIN="${CXX:-c++}"
PROBE="vcoCompileCanaryProbe"

# Scratch dir for object files and synthetic translation units; removed on exit
# so a run never leaves residue in the working tree.
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

# The exact gate the canary must satisfy — same flags as the Makefile `strict`
# target and the CI "Strict C++11 pedantic gate" step, minus the Rack SDK
# -isystem paths (the canary includes no Rack header, by design).
STRICT_FLAGS=(-std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Wno-unused-parameter -I"${ROOT}/src")

fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }

# ---------------------------------------------------------------------------
# [1/5] The canary exists and names its probe.
# ---------------------------------------------------------------------------
echo "[1/5] Canary exists and declares + defines its probe..."
if [[ ! -f "${CANARY}" ]]; then
	note_fail "missing ${CANARY_REL} — the D-07 canary is gone; make strict and the CI MinGW leg now cover ZERO VCO code"
else
	echo "  OK: ${CANARY_REL} present"
	# Strip whole-line comments so the banner (which names the probe in prose)
	# cannot satisfy either check on its own.
	CANARY_CODE="$(grep -v '^[[:space:]]*//' "${CANARY}" || true)"
	if printf '%s\n' "${CANARY_CODE}" | grep -qE "${PROBE}[[:space:]]*\(.*\)[[:space:]]*;"; then
		echo "  OK: forward declaration of ${PROBE} present (this is what forces external linkage)"
	else
		note_fail "no forward declaration of ${PROBE} — without it a compiler may prove the definition unreachable and discard it"
	fi
	if printf '%s\n' "${CANARY_CODE}" | grep -qE "${PROBE}[[:space:]]*\(.*\)[[:space:]]*\{"; then
		echo "  OK: definition of ${PROBE} present"
	else
		note_fail "no definition of ${PROBE} in ${CANARY_REL}"
	fi
fi

# ---------------------------------------------------------------------------
# [2/5] The canary EMITS code (Pitfall P-1 guard).
#
# A translation unit that only #includes a header emits nothing: header-defined
# inline and member functions are instantiated only when used, so no in-class
# static constexpr is ever odr-used and the MinGW link leg has nothing to
# resolve. Such a canary is permanently and silently green. This section fails
# the build the moment that happens.
#
# The symbol name is C++-MANGLED and the leading-underscore convention differs
# between macOS (__ZN5forge...) and Linux (_ZN5forge...), which is why this
# greps for the probe name as a SUBSTRING of the mangled symbol rather than
# matching an exact symbol string. The nm type letter must be T/t/W/w — a
# DEFINED symbol. A 'U' (undefined) entry would mean the probe was only
# referenced, not emitted.
# ---------------------------------------------------------------------------
echo "[2/5] Canary emits a defined external-linkage probe symbol (P-1 guard)..."
if ! command -v nm >/dev/null 2>&1; then
	note_fail "nm not found — cannot prove the canary emits code, and a guard that cannot check is not a guard"
elif [[ ! -f "${CANARY}" ]]; then
	note_fail "skipped: canary missing (see [1/5])"
elif ! "${CXX_BIN}" -std=c++11 -O3 -I"${ROOT}/src" -c "${CANARY}" -o "${TMP}/canary.o" 2>"${TMP}/emit_err.txt"; then
	note_fail "canary failed to compile to an object file at -O3:"
	sed 's/^/    /' "${TMP}/emit_err.txt"
elif nm "${TMP}/canary.o" | grep -qE " [TtWw] [^ ]*${PROBE}"; then
	echo "  OK: defined symbol emitted at -O3 — $(nm "${TMP}/canary.o" | grep -E " [TtWw] [^ ]*${PROBE}" | awk '{print $2, $3}' | head -1)"
else
	note_fail "no DEFINED ${PROBE} symbol in the canary object — the translation unit emits no code, so it odr-uses nothing and the CI MinGW link leg has nothing to resolve. This canary would be permanently and silently green. Restore the external-linkage forward declaration and the runtime-derived loop trip count."
fi

# ---------------------------------------------------------------------------
# [3/5] The canary passes the C++11 pedantic gate.
# ---------------------------------------------------------------------------
echo "[3/5] Canary compiles clean under -std=c++11 -pedantic-errors..."
if [[ ! -f "${CANARY}" ]]; then
	note_fail "skipped: canary missing (see [1/5])"
elif "${CXX_BIN}" "${STRICT_FLAGS[@]}" "${CANARY}" 2>"${TMP}/strict_err.txt"; then
	echo "  OK: clean under the toolchain's standard"
else
	note_fail "canary does not satisfy the C++11 pedantic gate:"
	sed 's/^/    /' "${TMP}/strict_err.txt"
fi

# ---------------------------------------------------------------------------
# [4/5] NEGATIVE CONTROL — prove the C++17-ism gate actually bites.
#
# A guard that has only ever been green is unvalidated. Each synthetic
# translation unit below includes dsp/VcoCore.hpp plus exactly ONE C++17-only
# construct and is compiled with the SAME flags as [3/5]. If any of the three
# hard-requirement units COMPILES, the gate is not biting and this script fails.
#
# The [[maybe_unused]] unit is informational only: Apple clang rejects it under
# these flags, but GCC may merely ignore an unknown attribute with a warning.
# Making it a hard requirement would produce a false failure on the CI runner.
# ---------------------------------------------------------------------------
echo "[4/5] NEGATIVE CONTROL — C++17-isms must be rejected..."

cat > "${TMP}/nc_inline_constexpr.cpp" <<'EOF'
#include "dsp/VcoCore.hpp"
// C++17 inline variable at namespace scope.
inline constexpr int kCanaryNegativeControl = 42;
float ncProbe() { return static_cast<float>(kCanaryNegativeControl); }
EOF

cat > "${TMP}/nc_if_constexpr.cpp" <<'EOF'
#include "dsp/VcoCore.hpp"
// C++17 compile-time if.
template <typename T>
float ncProbe(T v) {
	if constexpr (sizeof(T) > 1) {
		return static_cast<float>(v);
	} else {
		return 0.f;
	}
}
float ncCall() { return ncProbe<int>(3); }
EOF

cat > "${TMP}/nc_std_clamp.cpp" <<'EOF'
#include <algorithm>
#include "dsp/VcoCore.hpp"
// C++17 <algorithm> addition.
float ncProbe(float v) { return std::clamp(v, 0.f, 1.f); }
EOF

cat > "${TMP}/nc_maybe_unused.cpp" <<'EOF'
#include "dsp/VcoCore.hpp"
// C++17 standard attribute.
float ncProbe(int i) { [[maybe_unused]] int unusedLocal = i; return 0.f; }
EOF

# label | file | mode (hard = must be rejected; info = report only)
check_negative_control() {
	nc_label="$1"
	nc_file="$2"
	nc_mode="$3"
	if "${CXX_BIN}" "${STRICT_FLAGS[@]}" "${nc_file}" 2>/dev/null; then
		if [[ "${nc_mode}" == "hard" ]]; then
			note_fail "${nc_label}: ACCEPTED by the C++11 pedantic gate — the gate is not biting, so a C++17-ism could reach the VCV Library toolchain unnoticed"
		else
			echo "  INFO: ${nc_label}: accepted by this compiler (informational only — GCC may merely warn; not a failure)"
		fi
	else
		echo "  OK: ${nc_label}: rejected by the C++11 pedantic gate"
	fi
}

check_negative_control "namespace-scope 'inline constexpr' variable" "${TMP}/nc_inline_constexpr.cpp" hard
check_negative_control "'if constexpr' statement"                    "${TMP}/nc_if_constexpr.cpp"     hard
check_negative_control "'std::clamp' call"                           "${TMP}/nc_std_clamp.cpp"        hard
check_negative_control "'[[maybe_unused]]' attribute"                "${TMP}/nc_maybe_unused.cpp"     info

# ---------------------------------------------------------------------------
# [5/5] D-08 growth rule — every VCO header is carried by the canary.
#
# This turns "each later phase adds its include" from a convention into a gate.
# Comment lines are stripped first, so the commented Phase 32 placeholder in the
# canary does NOT satisfy the check once dsp/MorphBlep.hpp actually exists.
# ---------------------------------------------------------------------------
echo "[5/5] D-08 growth rule — canary includes every VCO header..."
if [[ ! -f "${CANARY}" ]]; then
	note_fail "skipped: canary missing (see [1/5])"
else
	CANARY_CODE="$(grep -v '^[[:space:]]*//' "${CANARY}" || true)"
	VCO_HEADERS=()
	for h in "${ROOT}"/src/dsp/Vco*.hpp; do
		[[ -f "${h}" ]] && VCO_HEADERS+=("${h}")
	done
	# Phase 32's morph-BLEP header joins the list the moment it lands.
	[[ -f "${ROOT}/src/dsp/MorphBlep.hpp" ]] && VCO_HEADERS+=("${ROOT}/src/dsp/MorphBlep.hpp")

	if [[ "${#VCO_HEADERS[@]}" -eq 0 ]]; then
		note_fail "no src/dsp/Vco*.hpp found at all — the VCO seam has vanished"
	fi
	for h in "${VCO_HEADERS[@]}"; do
		base="$(basename "${h}")"
		if printf '%s\n' "${CANARY_CODE}" | grep -qE "#include[[:space:]]*\"dsp/${base}\""; then
			echo "  OK: dsp/${base} is carried into both gates by the canary"
		else
			note_fail "dsp/${base} is NOT included by ${CANARY_REL} (D-08). That header reaches neither the C++11 strict gate nor the CI MinGW link leg — add '#include \"dsp/${base}\"' to the canary."
		fi
	done
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${fail}" -eq 0 ]]; then
	echo "PASS: VCO compile canary guard clean (emits code + C++11 clean + C++17-isms rejected + D-08 complete)."
	exit 0
else
	echo "FAIL: VCO compile canary guard found problems (see above)."
	exit 1
fi
