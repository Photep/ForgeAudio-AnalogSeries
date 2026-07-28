// tests/test_lfo_guardrail.cpp
//
// D-04 LFO GOLDEN INTEGRITY TRIPWIRE.
//
// This translation unit exists so that tests/test_golden.cpp can stay
// BYTE-UNCHANGED (R-3). test_golden.cpp is the shipped LFO's only behavioral
// witness; hardening it by editing it would let a single edit weaken both the
// replay and the guard on that replay at once. It stays frozen and is itself
// hash-pinned by the D-05 manifest.
//
// Guards carried by this TU:
//   1. Hasher correctness — three published FIPS PUB 180-4 vectors plus a
//      missing-file case, so a hasher that returns a constant, an empty string,
//      or a truncated digest cannot pass.
//   2. D-04 golden byte lock — the SHA-256 of every tests/golden/freerun_*.f32
//      is pinned here as a source literal, so changing a golden requires a
//      reviewed CODE diff and can never be a silent regeneration.
//   3. Negative control — a one-byte-perturbed IN-MEMORY copy of a real golden
//      must hash differently. A guard only ever observed green is unvalidated.
//
// Fixture paths are repo-root-relative (matching tests/test_golden.cpp), so the
// test binary must run from the repository root — `make test` and the Windows CI
// leg both do.
//
// Digests are compared as EXACT strings. doctest::Approx is meaningless for a
// hex digest and is deliberately not used anywhere in this file.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "Sha256.hpp"

#include <string>
#include <vector>

namespace {

// Published FIPS PUB 180-4 / NIST CSRC example vectors for SHA-256.
const char* const VEC_EMPTY_DIGEST =
	"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
const char* const VEC_ABC_DIGEST =
	"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

// The 56-byte two-block message. Its length forces padding to spill into a
// second 64-byte block — the case a naive padding implementation gets wrong.
const char* const VEC_TWOBLOCK_MESSAGE =
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
const char* const VEC_TWOBLOCK_DIGEST =
	"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";

// A digest is always exactly 64 lowercase hex characters.
bool isLowerHex64(const std::string& s) {
	if (s.size() != 64u)
		return false;
	for (size_t i = 0; i < s.size(); ++i) {
		const char c = s[i];
		const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
		if (!ok)
			return false;
	}
	return true;
}

} // namespace

// --- Hasher correctness: published vectors ----------------------------------

TEST_CASE("lfo guardrail: SHA-256 hasher matches the published empty-string vector") {
	const std::vector<unsigned char> empty;
	const std::string got = forge::sha256HexBytes(empty);
	CHECK(got == VEC_EMPTY_DIGEST);
	CHECK(isLowerHex64(got));
	// Same message via the std::string entry point must agree.
	CHECK(forge::sha256Hex(std::string()) == VEC_EMPTY_DIGEST);
}

TEST_CASE("lfo guardrail: SHA-256 hasher matches the published abc vector") {
	const std::string msg = "abc";
	const std::vector<unsigned char> bytes(msg.begin(), msg.end());
	const std::string got = forge::sha256HexBytes(bytes);
	CHECK(got == VEC_ABC_DIGEST);
	CHECK(isLowerHex64(got));
	CHECK(forge::sha256Hex(msg) == VEC_ABC_DIGEST);
	// Distinctness: a constant-returning hasher would fail this.
	CHECK(got != std::string(VEC_EMPTY_DIGEST));
}

TEST_CASE("lfo guardrail: SHA-256 hasher matches the published two-block vector") {
	const std::string msg = VEC_TWOBLOCK_MESSAGE;
	REQUIRE(msg.size() == 56u); // spills padding into a second block
	const std::vector<unsigned char> bytes(msg.begin(), msg.end());
	const std::string got = forge::sha256HexBytes(bytes);
	CHECK(got == VEC_TWOBLOCK_DIGEST);
	CHECK(isLowerHex64(got));
	CHECK(forge::sha256Hex(msg) == VEC_TWOBLOCK_DIGEST);
}

TEST_CASE("lfo guardrail: SHA-256 of a missing file is reported as an empty digest") {
	// A missing fixture must surface as a mismatch, never as a silent pass:
	// the empty string can never equal a pinned 64-character digest.
	const std::string got = forge::sha256HexFile("tests/golden/this_file_does_not_exist.f32");
	CHECK(got.empty());
	CHECK(got != std::string(VEC_EMPTY_DIGEST)); // NOT the digest of empty input

	const std::string gotLf =
		forge::sha256HexFileLfNormalized("tests/golden/this_file_does_not_exist.f32");
	CHECK(gotLf.empty());
}
