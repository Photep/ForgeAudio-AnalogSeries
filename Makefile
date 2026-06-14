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
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk

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
