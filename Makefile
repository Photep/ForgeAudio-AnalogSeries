RACK_DIR ?= ../Rack-SDK

# -Isrc lets the shell's #include "dsp/LfoCore.hpp" and the headers' internal
# #include "dsp/*.hpp" resolve (the extracted core is consumed by both the plugin
# and the Rack-free test target, which already passes -Isrc).
FLAGS += -Isrc
CFLAGS +=
CXXFLAGS +=
LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard NOTICES*)
DISTRIBUTABLES += $(wildcard presets)

# `make test` is Rack-free (TEST-01 / D-09). A bare `include` hard-fails when
# ../Rack-SDK is absent (e.g. GitHub Actions ubuntu/macos runners), so skip it
# when `test` is the goal — the TEST_-namespaced target needs nothing from
# plugin.mk and $(CXX) falls back to the make default (CR-01).
# `guards` joins the filter for the same reason (R-11 / P-6): the guard suite is
# pure shell plus a checksum tool and must run on a runner with no Rack SDK.
# `audition` joins it for the same reason again (plan 33-10 / D-15): the A/B
# renderer drives forge::VcoCore, which is Rack-free by construction, so the
# target must run on a machine with no ../Rack-SDK checked out — exactly like
# its three siblings above. Omitting it here would hard-fail the target on a
# runner with a `No such file or directory` from the bare include, which is a
# failure about the SDK rather than about the renderer.
ifeq ($(filter test capture guards audition,$(MAKECMDGOALS)),)
include $(RACK_DIR)/plugin.mk
endif

# ---------------------------------------------------------------------------
# Standalone test harness (TEST-01) — purely additive.
# `make`, `make dist`, `make install` are unchanged: plugin.mk defines no `test`
# target and all variables below are TEST_-namespaced. This target links NO
# libRack and uses NO -I$(RACK_DIR)/include (RACK_DIR is irrelevant to `make test`).
# ---------------------------------------------------------------------------
TEST_DIR      := tests
TEST_BIN      := build-test/test
TEST_SOURCES  := $(wildcard $(TEST_DIR)/*.cpp)
TEST_HEADERS  := $(wildcard src/dsp/*.hpp) $(wildcard $(TEST_DIR)/*.hpp)
# -Isrc lets tests #include "dsp/LfoCore.hpp"; -I$(TEST_DIR) finds doctest.h.
# NO -I$(RACK_DIR)/include. No -ffast-math; -ffp-contract=off for cross-platform bit stability.
TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o $@

# ---------------------------------------------------------------------------
# Drift-off golden generator (D-07 / TEST-06) — one-shot, NOT wired into `test`.
# Rack-free like `test:` (added to the plugin.mk skip filter above). Compiles with
# the SAME TEST_CXXFLAGS so captured fixtures are bit-identical to what `make test`
# replays. Run from the repo root — output paths are relative to CWD.
# ---------------------------------------------------------------------------
CAPTURE_BIN := build-test/capture

.PHONY: capture
capture: $(CAPTURE_BIN)
	./$(CAPTURE_BIN)

$(CAPTURE_BIN): tools/capture_golden.cpp $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) tools/capture_golden.cpp -o $@

# ---------------------------------------------------------------------------
# Hard-sync A/B audition renderer (Phase 33, plan 33-10 — D-13 / D-14 / D-15 /
# D-16). One-shot, NOT wired into `test`, and modelled line for line on the
# `capture` target above, which is this tree's only other standalone tool.
#
# WHAT IT IS FOR. Phase 32's operator audition asked whether an improvement was
# AUDIBLE and supplied no reference to compare against, so that half of the
# verdict was unanswerable BY CONSTRUCTION (register item 26; the operator's own
# reply was "it's hard to remember what the old audio sounded like"). This target
# puts the reference in the room: a matched pair per render point, the shipped
# band-limited sync leg and the same reset with the sync correction withheld,
# from the SAME PASS through the real forge::VcoCore.
#
# THE OUTPUT IS NEVER COMMITTED (D-15). It lands under build-test/, which
# .gitignore already ignores, so this costs ZERO ignore-file edits and the
# "generated on demand, never a pinned golden" rule is enforced BY CONSTRUCTION
# rather than by anybody's discipline. A rendered pair captured from one
# toolchain must never become a reference — every decibel and every volt in this
# phase is an Apple-clang figure.
#
# IT COMPILES WITH TEST_CXXFLAGS, AND THAT IS LOAD-BEARING, NOT TIDINESS. The
# same standard, the same -O2, the same -ffp-contract=off. It is what makes the
# rendered audio bit-comparable with what `make test` measures, so the operator
# is listening to the same arithmetic the 420-cell sync grid and the SC-3
# time-domain gate report on. A fresh flags variable here would silently decouple
# the two — -ffast-math or a contracted FMA alone would move the samples.
#
# WHY THE RENDERER IS NOT IN tests/. TEST_SOURCES is a wildcard over that whole
# directory, so a tools-shaped .cpp placed there would be linked into the doctest
# binary and its main() would collide with doctest's — and even if it did not, it
# would run on EVERY `make test` invocation, which contradicts the
# generated-on-demand decision outright. It also costs a tests/check_includes.sh
# VCO_SIDE_ALLOW entry either way (paid in the same commit as this target).
#
# GNU Make 3.81 compatible, like every target above: plain rules, no $(file ...),
# no ::=, no .ONESHELL.
# ---------------------------------------------------------------------------
AUDITION_BIN := build-test/audition-render
AUDITION_OUT := build-test/audition

.PHONY: audition
audition: $(AUDITION_BIN)
	@mkdir -p $(AUDITION_OUT)
	./$(AUDITION_BIN)

$(AUDITION_BIN): tools/render_sync_ab.cpp $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) tools/render_sync_ab.cpp -o $@

# ---------------------------------------------------------------------------
# Submission preflight (post-v2.0.0-rejection lesson — see RETROSPECTIVE.md).
# The VCV library toolchain builds every platform with -std=c++11 (GCC on
# win/linux); local clang at -O3 masks C++17-isms and ODR'd in-class static
# constexpr. This strict-compiles the plugin sources to the toolchain's
# standard — run before every tag/submission. SDK headers are -isystem so
# only OUR code is held to -pedantic-errors. CI mirrors this plus a full
# MinGW link (the ODR class only surfaces at link time).
# ---------------------------------------------------------------------------
.PHONY: strict
strict:
	$(CXX) -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Wno-unused-parameter \
		-Isrc -isystem $(RACK_DIR)/include -isystem $(RACK_DIR)/dep/include $(wildcard src/*.cpp)
	@echo "strict C++11 gate: PASS"

# ---------------------------------------------------------------------------
# LFO non-regression guard suite (Phase 29) — purely additive.
#
# One local command for every standing guard this milestone adds, so the D-05
# and D-06 negative controls are reproducible off CI instead of only in it.
# `make`, `make dist`, `make install`, `make test`, `make capture` and
# `make strict` are all unchanged: every variable below is GUARD_-namespaced and
# this target deliberately shares NOTHING with TEST_CXXFLAGS (R-4) — a guard that
# could perturb the test compiler flags could move the golden float results it
# exists to protect.
#
# Rack-free: needs only bash, a checksum tool and (for the canary guard) a C++
# compiler. `guards` is in the plugin.mk skip filter above, so this runs on a
# machine with no ../Rack-SDK (R-11 / P-6).
#
# GNU Make 3.81 compatible — a plain shell `for` loop, no $(file ...), no ::=,
# no reliance on .ONESHELL.
# ---------------------------------------------------------------------------
GUARD_SCRIPTS := tests/check_frozen.sh tests/check_includes.sh tests/check_canary.sh

.PHONY: guards
guards:
	@for s in $(GUARD_SCRIPTS); do \
		echo "=== $$s ==="; \
		bash $$s || exit 1; \
	done
	@echo "guard suite: PASS"
