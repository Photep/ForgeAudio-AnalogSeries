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

#include <cstddef>
#include <cstdio>   // std::remove, for the CRLF scratch files
#include <fstream>
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

// ===========================================================================
// D-04 GOLDEN BYTE LOCK — THESE BYTES ARE FROZEN.
// ===========================================================================
//
// Each entry pins the SHA-256 of one shipped-LFO golden fixture. These files
// back the six behavioral replays in tests/test_golden.cpp, which are the
// regression canary for a plugin that is LIVE in the VCV Library.
//
// A golden may change ONLY through a deliberate, reviewed edit of THIS FILE.
// If a replay in tests/test_golden.cpp goes red, the LFO's behavior changed —
// that is the finding, not a stale fixture. NEVER run `make capture` to make a
// failing replay green: regenerating the fixtures overwrites the evidence and
// silently launders a real regression into a "new baseline".
//
// Paths are repo-root-relative, matching the literals in tests/test_golden.cpp.
// Digests below were recomputed from the working tree with `shasum -a 256` and
// agree with 29-RESEARCH.md and tests/golden/SHA256SUMS.
struct GoldenFixture {
	const char* path;
	const char* digest;
};

const GoldenFixture GOLDENS[] = {
	{"tests/golden/freerun_44100.f32",
	 "86f110db82efafc140d6ebc4e13a3015c30afcc3ba761d596d3a3855a01f16c7"},
	{"tests/golden/freerun_44100_driftoff.f32",
	 "cf947ae18b32c4a52c1dfbb48e7a26466ac43bcc245319d999a124ecc2f3b1a5"},
	{"tests/golden/freerun_48000.f32",
	 "51e274fe2c2477da0ba71a1acdd97eca2bd9dd7ff421237a03530e1f9e0e77c8"},
	{"tests/golden/freerun_48000_driftoff.f32",
	 "e3ed634ef50352fd6b81288bb548bb73079521009168deaf1aa5ac4164118be5"},
	{"tests/golden/freerun_96000.f32",
	 "a450d0963e5eda8fcba15084978f49e3bc22d9d6001104d81432b4f181229b74"},
	{"tests/golden/freerun_96000_driftoff.f32",
	 "b935779570067988a23282c60d2e6a33b4ea691f4f31b36a4d36ecdf07be3af2"}
};

const size_t GOLDEN_COUNT = sizeof(GOLDENS) / sizeof(GOLDENS[0]);

// 8192 float32 samples per fixture (tests/golden/freerun_seeds.txt).
const size_t GOLDEN_BYTES = 32768u;

// Look the pinned digest up by path so a table reorder cannot silently
// mis-pair a fixture with someone else's digest.
std::string pinnedDigestFor(const std::string& path) {
	for (size_t i = 0; i < GOLDEN_COUNT; ++i) {
		if (path == std::string(GOLDENS[i].path))
			return std::string(GOLDENS[i].digest);
	}
	return std::string();
}

// Raw byte reader. Mirrors the std::ios::binary relative-path shape of
// tests/test_golden.cpp's loadF32(), but reads bytes rather than floats.
std::vector<unsigned char> readAllBytes(const std::string& path) {
	std::vector<unsigned char> v;
	std::ifstream f(path.c_str(), std::ios::binary);
	if (!f.is_open())
		return v;
	char chunk[8192];
	while (f) {
		f.read(chunk, (std::streamsize)sizeof chunk);
		const std::streamsize n = f.gcount();
		if (n <= 0)
			break;
		for (std::streamsize i = 0; i < n; ++i)
			v.push_back((unsigned char)chunk[(size_t)i]);
	}
	return v;
}

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

TEST_CASE("lfo guardrail: SHA-256 streaming state machine is sealed by hex()") {
	// The behaviour this pins: multi-segment streaming must equal the one-shot
	// digest of the concatenation. update() used to return early and SILENTLY
	// after hex(), so a caller who streamed, read an intermediate digest, then
	// streamed more got the digest of the first segment only with no indication
	// anything had been dropped. That is now an assert, and sealed() lets the
	// seal be observed without tripping it.
	forge::Sha256 h;
	CHECK_FALSE(h.sealed());

	const std::string a = "a";
	const std::string bc = "bc";
	h.update(reinterpret_cast<const unsigned char*>(a.data()), a.size());
	CHECK_FALSE(h.sealed()); // still open between segments
	h.update(reinterpret_cast<const unsigned char*>(bc.data()), bc.size());
	CHECK_FALSE(h.sealed());

	const std::string first = h.hex();
	CHECK(h.sealed()); // hex() seals; a further update() would now assert

	// Segmented "a" + "bc" is the published "abc" vector, NOT sha256("a").
	CHECK(first == VEC_ABC_DIGEST);
	// hex() is idempotent and does not corrupt state on a second call.
	CHECK(h.hex() == first);
	CHECK(h.sealed());
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

TEST_CASE("lfo guardrail: LF-normalized file hashing actually ignores CR bytes") {
	// Before this case, the stripCarriageReturns == true branch of
	// detail::hashFileImpl had ZERO coverage and no production caller anywhere in
	// the repo. Its only call site was the missing-file case above, which returns
	// at the is_open() check before any hashing happens. The header presents this
	// function as the answer to pitfall P-3 (Windows CRLF checkouts) and
	// tests/check_frozen.sh rests on the same normalization idea in shell, so the
	// C++ half of that story was untested code that a later phase would trust.
	const char* const kLf = "tmp_sha256_crlf_probe_lf.txt";
	const char* const kCrlf = "tmp_sha256_crlf_probe_crlf.txt";
	const std::string body = "one\ntwo\nthree\n";

	{
		std::ofstream a(kLf, std::ios::binary);
		REQUIRE(a.is_open());
		a << body;
	}
	{
		std::ofstream b(kCrlf, std::ios::binary);
		REQUIRE(b.is_open());
		b << "one\r\ntwo\r\nthree\r\n";
	}

	const std::string lfNorm = forge::sha256HexFileLfNormalized(kLf);
	const std::string crlfNorm = forge::sha256HexFileLfNormalized(kCrlf);
	const std::string lfRaw = forge::sha256HexFile(kLf);
	const std::string crlfRaw = forge::sha256HexFile(kCrlf);

	REQUIRE(isLowerHex64(lfNorm));
	// The property P-3 rests on: line endings must not change the digest.
	CHECK(lfNorm == crlfNorm);
	// ...and it is not vacuous. If the RAW variant also ignored CR, the
	// normalization would be doing nothing and this case would prove nothing.
	CHECK(lfRaw != crlfRaw);
	// On an LF checkout the two entry points must agree, which is why
	// `shasum -a 256 -c src/dsp/FROZEN.sha256` still works by hand from the root.
	CHECK(lfNorm == lfRaw);
	// Cross-check against the in-memory entry point the published vectors above
	// already validate, so this does not merely compare the branch with itself.
	CHECK(lfNorm == forge::sha256Hex(body));

	std::remove(kLf);
	std::remove(kCrlf);
}

TEST_CASE("lfo guardrail: LF normalization holds across the 8192-byte read chunk boundary") {
	// hashFileImpl streams in 8192-byte chunks and filters each chunk separately,
	// so a file larger than one chunk exercises the branch's multi-chunk path —
	// including a CR that lands next to a chunk edge. The single-chunk case above
	// cannot detect a per-chunk state error.
	const char* const kLf = "tmp_sha256_crlf_chunk_lf.txt";
	const char* const kCrlf = "tmp_sha256_crlf_chunk_crlf.txt";

	std::string lfBody;
	std::string crlfBody;
	for (int i = 0; i < 4000; ++i) { // ~20 KB: several chunks either way
		lfBody += "line\n";
		crlfBody += "line\r\n";
	}
	REQUIRE(lfBody.size() > 8192u);
	REQUIRE(crlfBody.size() > 8192u);

	{
		std::ofstream a(kLf, std::ios::binary);
		REQUIRE(a.is_open());
		a << lfBody;
	}
	{
		std::ofstream b(kCrlf, std::ios::binary);
		REQUIRE(b.is_open());
		b << crlfBody;
	}

	const std::string lfNorm = forge::sha256HexFileLfNormalized(kLf);
	const std::string crlfNorm = forge::sha256HexFileLfNormalized(kCrlf);
	REQUIRE(isLowerHex64(lfNorm));
	CHECK(lfNorm == crlfNorm);
	CHECK(forge::sha256HexFile(kLf) != forge::sha256HexFile(kCrlf));
	CHECK(lfNorm == forge::sha256Hex(lfBody));

	std::remove(kLf);
	std::remove(kCrlf);
}

// --- D-04: the golden byte lock ---------------------------------------------

TEST_CASE("lfo guardrail: golden .f32 bytes are unchanged (D-04 checksum lock)") {
	for (size_t i = 0; i < GOLDEN_COUNT; ++i) {
		const std::string path = GOLDENS[i].path;
		CAPTURE(path);
		const std::string got = forge::sha256HexFile(path);
		// A missing or unreadable fixture must fail LOUDLY here, never slip
		// through as a silent pass further down.
		REQUIRE(!got.empty());
		CHECK(got == std::string(GOLDENS[i].digest));
	}
}

// --- The negative control ----------------------------------------------------
//
// DO NOT DELETE THIS CASE. A guard that has only ever been observed green is
// unvalidated: it proves the hasher ran, not that it would notice a change.
// This case perturbs a single bit of an IN-MEMORY copy of a real golden and
// requires the digest to move. It is what makes the D-04 lock *validated*
// rather than merely green. The on-disk fixture is never written.
TEST_CASE("lfo guardrail: golden hash lock detects a single-byte change (negative control)") {
	const std::string path = "tests/golden/freerun_44100_driftoff.f32";
	const std::string pinned = pinnedDigestFor(path);
	REQUIRE(!pinned.empty());

	const std::vector<unsigned char> bytes = readAllBytes(path);
	REQUIRE(bytes.size() == GOLDEN_BYTES);

	// Positive leg: the untouched buffer reproduces the pinned digest.
	CHECK(forge::sha256HexBytes(bytes) == pinned);

	// Negative leg: flip the low bit of one byte near the middle of the copy.
	std::vector<unsigned char> perturbed = bytes;
	const size_t idx = perturbed.size() / 2;
	perturbed[idx] = (unsigned char)(perturbed[idx] ^ (unsigned char)0x01u);
	REQUIRE(perturbed.size() == bytes.size());
	CHECK(perturbed[idx] != bytes[idx]);
	CHECK(forge::sha256HexBytes(perturbed) != pinned);
}

// --- Length assertions -------------------------------------------------------
//
// Catches a truncated fixture before the digest comparison, so the failure
// message says "wrong length" instead of only "wrong hash".
TEST_CASE("lfo guardrail: golden fixtures are the expected 32768-byte length") {
	for (size_t i = 0; i < GOLDEN_COUNT; ++i) {
		const std::string path = GOLDENS[i].path;
		CAPTURE(path);
		const std::vector<unsigned char> bytes = readAllBytes(path);
		CHECK(bytes.size() == GOLDEN_BYTES);
	}
}
