#!/usr/bin/env bash
#
# check_frozen.sh — the D-05 frozen-source tripwire for the shipped LFO.
#
# WHY THIS EXISTS
# ---------------
# v2.0 builds a VCO alongside a module that is already published in the VCV
# library (Forge Analog LFO, v2.0.1). Decision D-05 says the shared headers the
# LFO depends on are FROZEN for the duration of the VCO milestone: they may be
# extended additively by one named, sanctioned phase and by nothing else. A
# convention cannot enforce that. A checked-in SHA-256 manifest can. This script
# is the enforcement half: src/dsp/FROZEN.sha256 records the exact bytes of every
# frozen file, and this gate hard-fails the moment any of them changes without a
# deliberate manifest bump in the same commit.
#
# THE BUMP PROTOCOL (this is a feature, not a workaround)
# -------------------------------------------------------
# A frozen file may change ONLY through a deliberate edit accompanied by an
# updated digest line in src/dsp/FROZEN.sha256, in the SAME commit, so the change
# is visible in review instead of arriving as an invisible byte-level drift.
# Phase 34 performs exactly one such bump for src/dsp/DriftEngine.hpp — that is
# the sanctioned additive edit D-05 already anticipates. Any OTHER bump is a
# change to shipped LFO behavior and must be justified in the commit message.
#
# WHY THE MANIFEST IS LARGER THAN D-05'S TEXT (a strict superset, not a conflict)
# -------------------------------------------------------------------------------
# D-05 names four headers: Waveshape.hpp, RackCompat.hpp, MathConst.hpp and
# DriftEngine.hpp. The LFO's actual behavioral include closure is ELEVEN headers,
# and three of the seven D-05 does not name —
#
#     src/dsp/PatchParse.hpp, src/dsp/DisplayFill.hpp, src/dsp/Anim.hpp
#
# — have ZERO golden-fixture coverage AND are precisely the headers
# .planning/research/ARCHITECTURE.md flags as reuse candidates for the VCO shell
# in Phase 35. A literal four-header manifest would therefore leave the single
# most likely silent-regression surface in the entire milestone unguarded. The
# manifest is a strict superset of D-05: the four named headers, the rest of the
# closure, the shipped shell (src/AnalogLFO.cpp), and the two test files that are
# otherwise their own only witnesses — tests/BlockDriver.hpp (R-2) and
# tests/test_golden.cpp (R-3), which verify the goldens but which nothing
# verifies in turn. tests/golden/freerun_seeds.txt is pinned as the fixture
# provenance record (R-6). Every extra entry has the same one-line escape hatch
# D-05 already defines, so this extends the decision additively and contradicts
# nothing in it.
#
# DELIBERATELY NOT PINNED: plugin.json, src/plugin.cpp, src/plugin.hpp
# --------------------------------------------------------------------
# Phase 30 legitimately edits all three to register the VCO module, and the
# ROADMAP Phase 30 guardrail already requires that diff to be surfaced to the
# operator. Pinning them here would manufacture a mandatory manifest bump in the
# very phase that is designed to touch them — noise that trains a reader to bump
# without thinking, which is the one habit that would defeat this gate.
#
# CR NORMALIZATION (pitfall P-3)
# ------------------------------
# This repository has no .gitattributes, so a Windows checkout materializes every
# TEXT file with CRLF line endings and its raw digest differs from the LF digest
# recorded here. Text entries are therefore hashed over carriage-return-stripped
# bytes. On an LF checkout the CR-stripped digest is IDENTICAL to the raw digest,
# which is why `shasum -a 256 -c src/dsp/FROZEN.sha256` still works by hand from
# the repo root. Binary entries (the golden .f32 fixtures in section 2) are hashed
# RAW — stripping 0x0D from binary float data would be corruption, not
# normalization.
#
# Enforces:
#   [1/3] every file in src/dsp/FROZEN.sha256 matches its recorded digest
#         (CR-normalized, because the frozen set is all text)
#   [2/3] every file in tests/golden/SHA256SUMS matches its recorded digest
#         (raw bytes, because the fixtures are binary)
#   [3/3] NEGATIVE CONTROL — a one-line modification IS detected, proven on
#         every single run against a scratch copy
#
# Needs no Rack SDK and no compiler, runs from any working directory, and leaves
# no artifacts. Returns 0 (PASS) only when all three groups pass.

set -euo pipefail

# Resolve repo root relative to this script so it runs from anywhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

FROZEN_REL="src/dsp/FROZEN.sha256"
GOLDEN_REL="tests/golden/SHA256SUMS"

fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }

BUMP_HINT="If this change was intentional, update the digest line in ${FROZEN_REL} IN THE SAME COMMIT and say why in the commit message. That deliberate bump is the sanctioned path (it is exactly what Phase 34 does for src/dsp/DriftEngine.hpp). If it was NOT intentional, you have just changed the behavior of a module that is already published."

# ---------------------------------------------------------------------------
# Hasher detection.
#
# sha256sum is GNU coreutils and does NOT exist on macOS; shasum is Perl-based
# and ships with macOS but is absent from some minimal Linux images. openssl is
# the last resort. Detection order is macOS-first because that is the primary
# development machine. Each wrapper reads stdin and prints ONLY the 64-hex
# digest, so the callers below are hasher-agnostic. awk (rather than head)
# consumes the whole line, which avoids a SIGPIPE under `set -o pipefail`.
# ---------------------------------------------------------------------------
if command -v shasum >/dev/null 2>&1; then
	HASHER="shasum -a 256"
	hash_stdin() { shasum -a 256 | awk '{print $1}'; }
elif command -v sha256sum >/dev/null 2>&1; then
	HASHER="sha256sum"
	hash_stdin() { sha256sum | awk '{print $1}'; }
elif command -v openssl >/dev/null 2>&1; then
	HASHER="openssl dgst -sha256"
	hash_stdin() { openssl dgst -sha256 | awk '{print $NF}'; }
else
	echo "FAIL: no SHA-256 tool found (tried shasum, sha256sum, openssl)."
	echo "      A guard that cannot compute its own check is not a guard."
	exit 1
fi

# CR-stripped digest (text entries). Deliberately NOT the checksum tool's own
# -c/--check mode: that mode reads raw bytes and cannot normalize, so on a
# Windows CRLF checkout every text entry would report a false mismatch.
hash_norm() { tr -d '\r' < "$1" | hash_stdin; }

# Raw digest (binary entries).
hash_raw() { hash_stdin < "$1"; }

echo "using hasher: ${HASHER}"

# ---------------------------------------------------------------------------
# [1/3] Frozen source manifest — the D-05 tripwire.
# ---------------------------------------------------------------------------
echo "[1/3] Frozen source manifest (${FROZEN_REL}, CR-normalized)..."
if [[ ! -f "${ROOT}/${FROZEN_REL}" ]]; then
	note_fail "missing ${FROZEN_REL} — the D-05 manifest is gone, so every frozen LFO file is now unguarded"
else
	frozen_count=0
	while read -r expected relpath; do
		[[ -z "${expected:-}" ]] && continue
		frozen_count=$((frozen_count + 1))
		target="${ROOT}/${relpath}"
		if [[ ! -f "${target}" ]]; then
			note_fail "pinned file is MISSING: ${relpath}. ${BUMP_HINT}"
			continue
		fi
		actual="$(hash_norm "${target}")"
		if [[ "${actual}" == "${expected}" ]]; then
			echo "  OK: ${relpath}"
		else
			note_fail "FROZEN FILE CHANGED: ${relpath}"
			echo "        expected: ${expected}"
			echo "        actual:   ${actual}"
			echo "        ${BUMP_HINT}"
		fi
	done < "${ROOT}/${FROZEN_REL}"
	echo "  (${frozen_count} pinned entries checked)"
fi

# ---------------------------------------------------------------------------
# [2/3] Golden fixture manifest — RAW bytes, no normalization.
#
# The .f32 fixtures are little-endian binary float dumps. A 0x0D byte inside a
# float is data, not a line ending, so `tr -d '\r'` here would corrupt the
# stream and produce a digest that matches nothing on any platform.
# ---------------------------------------------------------------------------
echo "[2/3] Golden fixture manifest (${GOLDEN_REL}, raw bytes)..."
if [[ ! -f "${ROOT}/${GOLDEN_REL}" ]]; then
	note_fail "missing ${GOLDEN_REL} — the golden fixtures have no integrity record"
else
	golden_count=0
	while read -r expected relpath; do
		[[ -z "${expected:-}" ]] && continue
		golden_count=$((golden_count + 1))
		target="${ROOT}/${relpath}"
		if [[ ! -f "${target}" ]]; then
			note_fail "pinned fixture is MISSING: ${relpath}"
			continue
		fi
		actual="$(hash_raw "${target}")"
		if [[ "${actual}" == "${expected}" ]]; then
			echo "  OK: ${relpath}"
		else
			note_fail "GOLDEN FIXTURE CHANGED: ${relpath}"
			echo "        expected: ${expected}"
			echo "        actual:   ${actual}"
			echo "        Goldens are regenerated ONLY by a deliberate \`make capture\` run. If you did not run it, the LFO's output has moved."
		fi
	done < "${ROOT}/${GOLDEN_REL}"
	echo "  (${golden_count} pinned fixtures checked)"
fi

# ---------------------------------------------------------------------------
# [3/3] NEGATIVE CONTROL — prove the comparison actually detects a change.
#
# A guard that has only ever been observed GREEN is unvalidated: an equality
# test that always returns true looks identical to a correct one until the day
# it matters. This section copies a real frozen header into a scratch directory,
# appends a single blank line, and REQUIRES the digest to differ. It runs on
# every invocation and must never be removed.
#
# The real file is never touched — everything happens under `mktemp -d`, removed
# by the EXIT trap.
# ---------------------------------------------------------------------------
echo "[3/3] NEGATIVE CONTROL — a one-line modification must be detected..."
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

NC_REL="src/dsp/MathConst.hpp"
nc_expected="$(awk -v p="${NC_REL}" '$2 == p {print $1}' "${ROOT}/${FROZEN_REL}" 2>/dev/null || true)"
if [[ -z "${nc_expected}" ]]; then
	note_fail "negative control cannot run: ${NC_REL} is not in ${FROZEN_REL}"
elif [[ ! -f "${ROOT}/${NC_REL}" ]]; then
	note_fail "negative control cannot run: ${NC_REL} is missing"
else
	cp "${ROOT}/${NC_REL}" "${TMP}/perturbed.hpp"
	echo "" >> "${TMP}/perturbed.hpp"
	nc_actual="$(hash_norm "${TMP}/perturbed.hpp")"
	if [[ "${nc_actual}" == "${nc_expected}" ]]; then
		note_fail "negative control DID NOT FIRE: a perturbed copy of ${NC_REL} hashed identically to the manifest entry. The comparison logic in [1/3] is broken and this gate is reporting a green it has not earned."
	else
		echo "  OK: perturbed copy of ${NC_REL} was detected as different"
		echo "      manifest: ${nc_expected}"
		echo "      perturbed: ${nc_actual}"
	fi
	# The real file must be untouched by the control.
	if [[ "$(hash_norm "${ROOT}/${NC_REL}")" != "${nc_expected}" ]]; then
		note_fail "the negative control modified the REAL ${NC_REL} — that is a bug in this script"
	else
		echo "  OK: the real ${NC_REL} is untouched (control ran entirely in ${TMP})"
	fi
fi
echo "  NOTE: this negative control is what makes this gate VALIDATED rather than"
echo "        merely green. It runs on every invocation. Do not remove it."

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${fail}" -eq 0 ]]; then
	echo "PASS: frozen-source gate clean (D-05 manifest + golden fixtures + negative control)."
	exit 0
else
	echo "FAIL: frozen-source gate found problems (see above)."
	exit 1
fi
