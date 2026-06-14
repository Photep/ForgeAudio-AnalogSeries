#pragma once
// src/dsp/PatchParse.hpp
//
// Non-throwing hex seed parse for dataFromJson (BUG-04 / CODE-REVIEW #4).
// Replaces the throwing std::stoull at AnalogLFO.cpp:244-245 so a hand-edited
// patch with malformed / over-long / empty seed strings cannot crash Rack on
// load. On any parse failure the caller keeps its existing (constructor-seeded)
// value — ASVS V5 input-validation + safe fallback.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes — this header
// must link in the Rack-free `make test` harness.

#include <cstdint>
#include <cstdlib>   // std::strtoull
#include <cerrno>

namespace forge {

// Parse a NUL-terminated hex string into `out`. Returns true only on a fully
// consumed, in-range value; false on NULL/empty input, non-hex content, a
// trailing garbage tail, or an out-of-range (ERANGE) value. Never throws.
inline bool parseSeedHex(const char* s, uint64_t& out) {
	if (!s || !*s) return false;                 // NULL / empty -> fail
	char* end = nullptr;
	errno = 0;
	unsigned long long v = std::strtoull(s, &end, 16);
	if (errno == ERANGE || end == s || *end != '\0') return false;  // over-long / non-hex tail
	out = (uint64_t)v;
	return true;
}

} // namespace forge
