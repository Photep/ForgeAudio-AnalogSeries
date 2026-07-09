#!/usr/bin/env bash
#
# check_docs.sh — the single automatable documentation gate for the manual.
#
# Enforces:
#   (1) D-06 / Pitfall 4 — no trademarked synth brand names anywhere in docs/
#   (2) DOC-01/DOC-02 — the docs hub + every wave-1 section file exists
#   (3) DOC-02 fact-check — Swing labels, ratio labels, and the CV voltage token
#       match the module source (src/AnalogLFO.cpp is the oracle)
#
# Returns 0 (PASS) only when all three groups pass. Any denylist hit, missing
# section file, or missing fact token fails the gate with a non-zero exit.
#
# Note: docs/panel.md is authored in plan 27-04 (wave 2) and is validated by that
# plan's checkpoint — it is intentionally NOT required here, so this script PASSES
# after wave-1 authoring (plans 27-01 / 27-02).

set -euo pipefail

# Resolve repo root relative to this script so it runs from anywhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCS="${ROOT}/docs"

fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }

# ---------------------------------------------------------------------------
# (1) Brand-name denylist — trademarked synth names must never appear in docs/.
# ---------------------------------------------------------------------------
echo "[1/3] Brand-name denylist (D-06)..."
DENYLIST='Minimoog|Moog|Roland|Juno|SH-101|Prophet|Oberheim|Korg'
if hits=$(grep -rniE "${DENYLIST}" "${DOCS}" 2>/dev/null); then
	note_fail "trademarked brand name(s) found in docs/:"
	echo "${hits}" | sed 's/^/    /'
else
	echo "  OK: zero denylist hits"
fi

# ---------------------------------------------------------------------------
# (2) File-existence asserts — hub + the 7 wave-1 section files (not panel.md).
# ---------------------------------------------------------------------------
echo "[2/3] Section files exist..."
REQUIRED_FILES=(
	"index.md"
	"engine-concept.md"
	"io-reference.md"
	"context-menu.md"
	"clock-sync.md"
	"install.md"
	"changelog.md"
	"license-credits.md"
)
for f in "${REQUIRED_FILES[@]}"; do
	if [[ -f "${DOCS}/${f}" ]]; then
		echo "  OK: docs/${f}"
	else
		note_fail "missing docs/${f}"
	fi
done

# ---------------------------------------------------------------------------
# (3) Fact-check — Swing labels, ratio labels, and CV voltage token in docs/.
# ---------------------------------------------------------------------------
echo "[3/3] Code-fact tokens present in docs/..."
FACT_TOKENS=(
	# 6 Swing labels (SWING_MENU_LABELS in src/AnalogLFO.cpp)
	"Straight 50%"
	"Light 54%"
	"Medium 58%"
	"Triplet 66%"
	"Heavy 71%"
	"Max 75%"
	# Ratio labels (RATIO_LABELS extremes + the two half-step ratios)
	"/16"
	"/1.5"
	"x1.5"
	"x16"
	# CV voltage convention token
	"5V"
)
for tok in "${FACT_TOKENS[@]}"; do
	if grep -rqF -- "${tok}" "${DOCS}" 2>/dev/null; then
		echo "  OK: '${tok}'"
	else
		note_fail "fact token not found in docs/: '${tok}'"
	fi
done

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${fail}" -eq 0 ]]; then
	echo "PASS: docs gate clean (denylist + section files + code facts)."
	exit 0
else
	echo "FAIL: docs gate found problems (see above)."
	exit 1
fi
