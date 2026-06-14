// tests/test_regression.cpp
//
// TEST-05 fail-before / pass-after regression pins for the Phase 23 bug fixes.
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).
// Headers reached via -Isrc / -Itests (see the Makefile test target).

#include "doctest.h"

#include <cstdint>

#include "dsp/PatchParse.hpp"   // BUG-04: forge::parseSeedHex (non-throwing hex parse)

// ---------------------------------------------------------------------------
// BUG-04 (CODE-REVIEW #4 / ASVS V5): a hand-edited patch with a malformed,
// over-long, or empty seed hex string must NEVER throw — parseSeedHex signals
// failure so dataFromJson can keep the existing seed instead of crashing Rack.
// ---------------------------------------------------------------------------

TEST_CASE("BUG-04: malformed seed string never throws, signals failure") {
	uint64_t v = 0;
	CHECK_FALSE(forge::parseSeedHex("zzzz", v));                   // non-hex
	CHECK_FALSE(forge::parseSeedHex("FFFFFFFFFFFFFFFFFFFF", v));   // out of range
	CHECK_FALSE(forge::parseSeedHex("", v));                       // empty
	CHECK(forge::parseSeedHex("C0FFEE", v));                       // valid round-trips
	CHECK(v == 0xC0FFEEull);
}
