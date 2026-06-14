// tests/test_regression.cpp
//
// TEST-05 fail-before / pass-after regression pins for the Phase 23 bug fixes.
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).
// Headers reached via -Isrc / -Itests (see the Makefile test target).

#include "doctest.h"

#include <cstdint>

#include "dsp/PatchParse.hpp"   // BUG-04: forge::parseSeedHex (non-throwing hex parse)
#include "dsp/RatioTable.hpp"   // BUG-02: forge::shouldReset (ratio-alignment cadence)

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

// ---------------------------------------------------------------------------
// BUG-02 (CODE-REVIEW #2 / SC4): x1.5 and ÷1.5 ratios must align to whole LFO
// cycles instead of truncating mid-cycle. forge::shouldReset must fire exactly
// on the per-ratio boundary (every BEATS_PER_ALIGN[idx] beats) and NEVER before.
//
// EXPECTED[15] is the beats-per-reset cadence the implementation MUST produce,
// set from the LOGGED audition decision in .planning/STATE.md ### Decisions:
//
//   - [Phase 23]: x1.5/÷1.5 audition — DECISION: adopt-table — rationale: in-Rack
//     listening ... confirmed the current cadence truncates mid-cycle ... the
//     proposed BEATS_PER_ALIGN table (x1.5 → every 2 beats, ÷1.5 → every 3 beats)
//     is preferred. Gates plan 23-05: apply the two-cell table swap
//     (idx 8 → 2, idx 6 → 3) ...
//
// DECISION = adopt-table ⇒ idx 6 (/1.5) resets every 3 beats, idx 8 (x1.5) every 2.
// (keep-current would have been {...,2,1,1,...} at idx 6/8.) The 13 other ratios
// are bit-identical to the pre-swap round(1/ratio) cadence under either outcome.
// Pre-swap header: idx6=2, idx8=1 → this case is RED until the table swap lands.
// ---------------------------------------------------------------------------

TEST_CASE("BUG-02: ratio alignment cadence (per logged audition decision)") {
	// adopt-table cadence (idx 6 = 3, idx 8 = 2). Indexed by ratioIdx.
	//                                 /16 /8 /6 /4 /3 /2 /1.5 x1 x1.5 x2 x3 x4 x6 x8 x16
	static const int EXPECTED[15] = {  16, 8, 6, 4, 3, 2,  3,  1,  2,  1, 1, 1, 1, 1, 1 };
	for (int idx = 0; idx < 15; ++idx) {
		int period = EXPECTED[idx];
		for (int b = 1; b < period; ++b)
			CHECK_FALSE(forge::shouldReset(idx, b));   // no early (mid-cycle) reset
		CHECK(forge::shouldReset(idx, period));        // resets exactly on the boundary
	}
}
