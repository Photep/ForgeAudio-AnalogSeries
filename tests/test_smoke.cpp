// tests/test_smoke.cpp — trivial passing smoke test proving the harness runs.
// This is a non-impl TU: it ONLY #includes doctest.h (the implementation macro lives
// solely in main.cpp). Linking this second TU against main.cpp exercises doctest's
// discovery + reporting across two translation units. Real invariant/golden tests
// land in later plans.
#include "doctest.h"

TEST_CASE("harness smoke: 1+1==2") {
    CHECK(1 + 1 == 2);
}
