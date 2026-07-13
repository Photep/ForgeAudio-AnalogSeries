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
ifeq ($(filter test capture,$(MAKECMDGOALS)),)
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
