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
#   [2b/5] the canary ODR-USES the seam under -O3 — every VcoInputs DSP field is
#         fed a RUNTIME value, so a runtime-indexed in-class `static constexpr`
#         inside step() cannot be constant-folded out of existence before the
#         MinGW linker sees it (this is the property [2/5] only claims to test)
#   [3/5] the canary compiles clean under -std=c++11 -pedantic-errors
#   [4/5] NEGATIVE CONTROL — the C++17-ism gate demonstrably REJECTS known-bad
#         synthetic translation units, so this guard is validated by an observed
#         red rather than by never having been anything but green
#   [5/5] D-08 growth rule — every src/dsp/Vco*.hpp is included by the canary
#   [5b/5] naming rule — every header in the VCO seam is Vco*-named (or an
#         allowed D-05 frozen shared header). Four separate guards key off that
#         filename token, so a VCO header named anything else is invisible to all
#         four at once while every one of them still reports PASS.
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
# [2b/5] The canary ODR-USES the seam, not merely emits a symbol.
#
# WHY [2/5] IS NOT ENOUGH. [2/5] greps nm for the PROBE symbol, which is emitted
# even when the whole body has been folded to `return 0.f`. Its failure message
# ("the translation unit emits no code, so it odr-uses nothing") therefore
# describes a property it does not actually test. This section tests it.
#
# THE DEFECT THIS CATCHES — measured, not theorised. If the canary hands
# VcoInputs only compile-time constants (and the NSDMI defaults ARE constants),
# then after inlining, -O2/-O3 constant-propagates every field into whatever
# step() does with it. An in-class `static constexpr float kTable[4]` indexed by
# in.pitchCV then folds to a literal and its symbol DISAPPEARS FROM THE OBJECT
# ENTIRELY — verified at -O3: zero kTable symbol of any kind, while the probe
# symbol [2/5] looks for is still happily present. That is the exact construct
# that got v2.0.0 rejected from the VCV Library, and with a constant-fed canary
# the MinGW link leg has nothing left to fail on: every gate reports PASS.
#
# A runtime-derived LOOP TRIP COUNT does not help. It preserves how many times
# step() is called; nothing INSIDE step() depends on it.
#
# HOW THIS TESTS IT. Build a perturbed copy of dsp/VcoCore.hpp in a scratch
# include dir carrying one in-class `static constexpr` table per float field of
# VcoInputs, each indexed at runtime by ITS OWN field, and compile the REAL
# canary against it at -O3. A table whose field carries a runtime value survives
# in the object (a defined local symbol on clang, an UNDEFINED reference on
# GCC — either way the odr-use reached the object and the linker has something
# to resolve). A table that vanished names a field the canary is feeding a
# constant. The field list is derived FROM the header, so a field added by a
# later phase is covered the moment it lands rather than when someone remembers.
#
# The real dsp/VcoCore.hpp is never touched: everything happens under mktemp -d,
# removed by the EXIT trap.
# ---------------------------------------------------------------------------
echo "[2b/5] Canary ODR-USES the seam under -O3 (constant-fold guard)..."
VCO_CORE_REL="src/dsp/VcoCore.hpp"
VCO_CORE="${ROOT}/${VCO_CORE_REL}"
if ! command -v nm >/dev/null 2>&1; then
	note_fail "nm not found — cannot prove the canary odr-uses the seam (see [2/5])"
elif [[ ! -f "${CANARY}" ]]; then
	note_fail "skipped: canary missing (see [1/5])"
elif [[ ! -f "${VCO_CORE}" ]]; then
	note_fail "skipped: ${VCO_CORE_REL} missing — the VCO seam has vanished"
else
	# Precondition. If this compiler folds a runtime-indexed in-class static
	# constexpr away even when the index is genuinely runtime, the per-field check
	# below could not tell a constant-fed canary from a compiler quirk. Report
	# that rather than failing on it — the CI GCC/MinGW leg is the gate that matters.
	mkdir -p "${TMP}/odrcap"
	cat > "${TMP}/odrcap/cap.cpp" <<'EOF'
// Self-contained: no VCO header, so this measures the COMPILER, not the seam.
struct CanaryOdrCapability {
	static constexpr float kCanaryOdrCapTable[4] = {0.f, 1.f, 2.f, 3.f};
	float pick(int i) const { return kCanaryOdrCapTable[i & 3]; }
};
float canaryOdrCapCall(int i);
float canaryOdrCapCall(int i) { CanaryOdrCapability p; return p.pick(i); }
EOF
	odr_capable=0
	if "${CXX_BIN}" -std=c++11 -O3 -c "${TMP}/odrcap/cap.cpp" -o "${TMP}/odrcap.o" 2>/dev/null; then
		# nm into a file, never into `grep -q`: an early-exiting grep can SIGPIPE nm
		# and `set -o pipefail` would then report a MATCH as a failure.
		nm "${TMP}/odrcap.o" > "${TMP}/odrcap_nm.txt" 2>/dev/null || true
		if grep -q 'kCanaryOdrCapTable' "${TMP}/odrcap_nm.txt"; then
			odr_capable=1
			echo "  OK: this compiler keeps a runtime-indexed in-class static constexpr at -O3"
		fi
	fi
	if [[ "${odr_capable}" -eq 0 ]]; then
		echo "  INFO: this compiler erases even a genuinely runtime-indexed in-class static"
		echo "        constexpr at -O3, so the per-field check below cannot distinguish anything"
		echo "        here. It still runs on the CI GCC leg, which is the one that matters."
	else
		# Derive the runtime-varying DSP fields from the header itself. sampleTime and
		# sampleRate are excluded on purpose: they are INJECTED timing, the canary sets
		# them from 44100 literals deliberately, and no DSP indexes a table with them.
		odr_fields="$(awk '
			/^[[:space:]]*struct[[:space:]]+VcoInputs[[:space:]]*\{/ { inb = 1; next }
			inb && /^[[:space:]]*\}/ { inb = 0 }
			inb && $1 == "float" && $3 == "=" { print $2 }
		' "${VCO_CORE}" | grep -vE '^(sampleTime|sampleRate)$' || true)"
		odr_field_count="$(echo ${odr_fields} | wc -w | tr -d ' ')"

		if [[ -z "${odr_fields}" ]]; then
			note_fail "could not derive a single float field from 'struct VcoInputs' in ${VCO_CORE_REL}. This check cannot run, so the constant-fold hole above is UNGUARDED — do not leave it in this state."
		else
			ODR_INC="${TMP}/odrseam"
			mkdir -p "${ODR_INC}/dsp"
			PERT="${ODR_INC}/dsp/VcoCore.hpp"
			: > "${PERT}"
			odr_struct_hit=0
			odr_step_hit=0
			while IFS= read -r pline || [[ -n "${pline}" ]]; do
				case "${pline}" in
				"struct VcoCore"*"{"*)
					printf '%s\n' "${pline}" >> "${PERT}"
					odr_n=0
					for f in ${odr_fields}; do
						odr_n=$((odr_n + 1))
						# Distinct values per table so identical constant data cannot be
						# merged into one symbol and mask a folded field.
						printf '\tstatic constexpr float kCanaryOdr_%s[4] = {%d.f, %d.f, %d.f, %d.f};\n' \
							"${f}" "$((odr_n * 4))" "$((odr_n * 4 + 1))" "$((odr_n * 4 + 2))" "$((odr_n * 4 + 3))" \
							>> "${PERT}"
					done
					odr_struct_hit=1
					;;
				*"float step(const VcoInputs& in)"*"{"*)
					# Wrap rather than rewrite: the real body is renamed and still called,
					# so this stays correct when Phase 30 replaces `return 0.f` with DSP.
					printf '\tfloat step(const VcoInputs& in) {\n\t\tfloat odrAcc = 0.f;\n' >> "${PERT}"
					for f in ${odr_fields}; do
						printf '\t\todrAcc += kCanaryOdr_%s[((int)(in.%s * 8.f)) & 3];\n' "${f}" "${f}" >> "${PERT}"
					done
					printf '\t\treturn stepCanaryOdrReal(in) + odrAcc;\n\t}\n' >> "${PERT}"
					printf '%s\n' "${pline/float step(/float stepCanaryOdrReal(}" >> "${PERT}"
					odr_step_hit=1
					;;
				*)
					printf '%s\n' "${pline}" >> "${PERT}"
					;;
				esac
			done < "${VCO_CORE}"

			# The canary is COPIED next to the perturbed header rather than compiled in
			# place. A quoted #include is resolved against the INCLUDING FILE'S OWN
			# DIRECTORY before any -I path, so compiling ${CANARY_REL} where it lives
			# would silently pick up the real src/dsp/VcoCore.hpp and this whole
			# section would be vacuously green — the precise failure mode it exists to
			# catch, one level up. dsp/DriftEngine.hpp and friends are not in the
			# scratch dsp/ dir, so they still fall through to -I${ROOT}/src.
			cp "${CANARY}" "${ODR_INC}/odr_canary.cpp"

			if [[ "${odr_struct_hit}" -ne 1 ]] || [[ "${odr_step_hit}" -ne 1 ]]; then
				note_fail "could not perturb ${VCO_CORE_REL}: expected a 'struct VcoCore {' line and a 'float step(const VcoInputs& in) {' line. The seam was renamed or reshaped, so this check silently covers nothing — update the two case patterns in this section."
			elif ! "${CXX_BIN}" -std=c++11 -O3 -I"${ODR_INC}" -I"${ROOT}/src" -c "${ODR_INC}/odr_canary.cpp" -o "${TMP}/odr_canary.o" 2>"${TMP}/odr_err.txt"; then
				note_fail "the canary does not compile against the perturbed seam at -O3, so this check cannot run:"
				sed 's/^/    /' "${TMP}/odr_err.txt"
			else
				nm "${TMP}/odr_canary.o" > "${TMP}/odr_nm.txt" 2>/dev/null || true
				odr_missing=""
				for f in ${odr_fields}; do
					grep -q "kCanaryOdr_${f}" "${TMP}/odr_nm.txt" || odr_missing="${odr_missing} ${f}"
				done
				if [[ -n "${odr_missing}" ]]; then
					note_fail "at -O3 the canary constant-folds these VcoInputs field(s) away:${odr_missing}"
					echo "        A runtime-indexed in-class \`static constexpr\` reached through any of"
					echo "        them emits NO symbol at all, so the CI MinGW link leg has nothing to"
					echo "        fail on and the exact failure class that got v2.0.0 REJECTED passes"
					echo "        every gate. Fix: feed each of those fields a value DERIVED FROM THE"
					echo "        RUNTIME PARAMETER in ${CANARY_REL} (see the LOAD-BEARING block there)."
					echo "        A runtime loop trip count is NOT sufficient — nothing inside step()"
					echo "        depends on it."
				else
					echo "  OK: all ${odr_field_count} VcoInputs DSP field(s) stay runtime-live through step() at -O3"
				fi
			fi
		fi
	fi
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

# label | file | mode (hard = must be rejected; info = report only) | expected diagnostic
#
# The fourth argument is load-bearing. This function used to treat ANY non-zero
# compiler exit as proof that "the C++11 pedantic gate rejected the C++17-ism",
# and threw the diagnostic away with 2>/dev/null. Any unrelated failure — a typo
# in a fixture, a renamed header, a missing include path — produced the same
# green. Verified: breaking dsp/VcoCore.hpp with a bogus #include made all four
# controls print "OK: ... rejected by the C++11 pedantic gate" without one of
# them reaching its C++17 construct.
#
# This section exists specifically to be validated by an OBSERVED RED rather than
# by never having been anything but green. A control that cannot tell the
# intended red from an accidental one does not deliver that. So each control now
# (a) requires its fixture to compile CLEAN at -std=c++17, which catches a broken
# fixture before it can masquerade as a rejection, and (b) requires the C++11
# diagnostic to actually name the construct under test.
#
# The expected-diagnostic patterns cover both spellings in play: Apple clang says
# "inline variables are a C++17 extension" / "constexpr if is a C++17 extension",
# GCC says "'inline' variables are only available with -std=c++17" / "'if
# constexpr' only available with -std=c++17". They are matched case-insensitively.
check_negative_control() {
	local nc_label="$1" nc_file="$2" nc_mode="$3" nc_expect="$4"

	# Sanity: the fixture must be VALID C++17. Otherwise a typo in it is
	# indistinguishable from the rejection this control is trying to observe.
	if ! "${CXX_BIN}" -std=c++17 -fsyntax-only -I"${ROOT}/src" "${nc_file}" 2>"${TMP}/nc_sanity.txt"; then
		note_fail "${nc_label}: the fixture does not compile even at -std=c++17, so its rejection at C++11 proves nothing about C++17-isms:"
		sed 's/^/    /' "${TMP}/nc_sanity.txt"
		return
	fi

	if "${CXX_BIN}" "${STRICT_FLAGS[@]}" "${nc_file}" 2>"${TMP}/nc_err.txt"; then
		if [[ "${nc_mode}" == "hard" ]]; then
			note_fail "${nc_label}: ACCEPTED by the C++11 pedantic gate — the gate is not biting, so a C++17-ism could reach the VCV Library toolchain unnoticed"
		else
			echo "  INFO: ${nc_label}: accepted by this compiler (informational only — GCC may merely warn; not a failure)"
		fi
	elif grep -qiE -- "${nc_expect}" "${TMP}/nc_err.txt"; then
		echo "  OK: ${nc_label}: rejected for the expected reason"
	else
		note_fail "${nc_label}: rejected, but NOT for the C++17 reason this control tests. The gate may be biting on something unrelated (a broken fixture, a renamed header), in which case this control is reporting a green it has not earned:"
		sed 's/^/    /' "${TMP}/nc_err.txt"
	fi
}

check_negative_control "namespace-scope 'inline constexpr' variable" "${TMP}/nc_inline_constexpr.cpp" hard "'?inline'?[[:space:]]+variables?"
check_negative_control "'if constexpr' statement"                    "${TMP}/nc_if_constexpr.cpp"     hard "constexpr[[:space:]]+if|if[[:space:]]+constexpr"
check_negative_control "'std::clamp' call"                           "${TMP}/nc_std_clamp.cpp"        hard "clamp"
check_negative_control "'[[maybe_unused]]' attribute"                "${TMP}/nc_maybe_unused.cpp"     info "maybe_unused"

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
# [5b/5] NAMING RULE — every header in the VCO seam must be Vco*-named.
#
# FOUR separate guards key off the same filename tokens Vco* / MorphBlep:
# check_includes.sh [1/7] leak detection, [2/7] Rack-free, [3/7] include hygiene,
# and [5/5] above. All four iterate a glob, so a future VCO header named anything
# else — dsp/OscCore.hpp, dsp/PitchChain.hpp, dsp/Antialias.hpp — is invisible to
# ALL FOUR SIMULTANEOUSLY, and every one of them still reports PASS because the
# glob simply matches fewer files. [5/5] guards against forgetting to INCLUDE a
# Vco-named header; nothing guarded against forgetting to NAME it Vco-something.
#
# So the naming convention becomes a gate. Every dsp/ header reached by the
# canary or by a VCO header must be either VCO-named or one of the D-05 frozen
# shared headers the VCO is explicitly allowed to consume (one-way, D-05/D-06).
# ---------------------------------------------------------------------------
echo "[5b/5] Naming rule — every header in the VCO seam is Vco*-named..."
if [[ ! -f "${CANARY}" ]]; then
	note_fail "skipped: canary missing (see [1/5])"
else
	seam_srcs=("${CANARY}")
	for h in "${ROOT}"/src/dsp/Vco*.hpp; do
		if [[ -f "${h}" ]]; then seam_srcs+=("${h}"); fi
	done
	if [[ -f "${ROOT}/src/dsp/MorphBlep.hpp" ]]; then
		seam_srcs+=("${ROOT}/src/dsp/MorphBlep.hpp")
	fi
	# Line comments are stripped first, so the commented Phase 32 placeholder in
	# the canary does not count as a seam edge.
	seam_includes="$(cat "${seam_srcs[@]}" \
		| grep -v '^[[:space:]]*//' \
		| sed -n 's/^[[:space:]]*#[[:space:]]*include[[:space:]]*"\(dsp\/[^"]*\)".*/\1/p' \
		| sort -u || true)"
	seam_bad=""
	for inc in ${seam_includes}; do
		b="$(basename "${inc}")"
		case "${b}" in
			# The VCO's own headers.
			Vco*|MorphBlep.hpp) ;;
			# The D-05 frozen shared headers the VCO is allowed to consume. This
			# list is the four D-05 names; anything else under src/dsp/ is LFO
			# internals and the VCO must not couple to it.
			DriftEngine.hpp|MathConst.hpp|RackCompat.hpp|Waveshape.hpp) ;;
			*) seam_bad="${seam_bad} ${inc}" ;;
		esac
	done
	if [[ -n "${seam_bad}" ]]; then
		note_fail "header(s) pulled into the VCO seam that are neither Vco*-named nor an allowed frozen shared header:${seam_bad}"
		echo "        A VCO header that is not Vco*-named is invisible to FOUR guards at once"
		echo "        (check_includes.sh [1/7] leak detection, [2/7] Rack-free, [3/7] include"
		echo "        hygiene, and [5/5] above), and all four keep reporting PASS because their"
		echo "        glob just matches fewer files. Rename it dsp/Vco*.hpp. If it is instead a"
		echo "        frozen LFO header the VCO legitimately needs, that is a D-05 coupling"
		echo "        decision — add it to the allowed list in this section and say why."
	else
		echo "  OK: every header in the VCO seam is Vco*-named or an allowed frozen shared header"
	fi
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
