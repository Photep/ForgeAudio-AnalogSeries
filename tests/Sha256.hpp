#pragma once
// tests/Sha256.hpp
//
// Minimal SHA-256, implemented from the specification in FIPS PUB 180-4
// ("Secure Hash Standard", NIST, August 2015) — sections 4.1.2 (functions),
// 4.2.2 (the 64 round constants) and 6.2 (the SHA-256 computation). Written
// in-repo on purpose: adding a third-party dependency to hash six test fixtures
// would be a far larger surface than the ~120 lines below.
//
// WHAT THIS IS NOT: this is an INTEGRITY TRIPWIRE for D-04, not a security
// control. It exists so that a change to tests/golden/freerun_*.f32 must be a
// deliberate, reviewed code diff instead of a silent `make capture`
// regeneration. It is not hardened against a motivated adversary, it is not
// constant-time, and nothing about the plugin's safety depends on it.
//
// PLACEMENT RULE (load-bearing): this file lives in tests/ and must NEVER be
// moved under src/ or otherwise enter the shipped plugin build graph. The
// release toolchain compiles src/*.cpp at -std=c++11 for three platforms; a
// hash used as a test tripwire has no business in a released audio plugin.
// tests/check_includes.sh asserts this placement.
//
// Written C++11-clean even though tests/ builds at C++17 (no std::clamp, no
// inline constexpr variables, no if constexpr, no structured bindings), matching
// house style in src/dsp/*.hpp so a future relocation stays cheap.
//
// Validated by tests/test_lfo_guardrail.cpp against three published FIPS 180-4
// vectors plus a one-byte perturbation negative control.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace forge {

// Streaming SHA-256 state. Feed bytes with update(), read the digest with hex().
// hex() finalizes on first call and is idempotent: calling it twice returns the
// same string and never corrupts the state (guarded by `finalized`).
struct Sha256 {
	void update(const unsigned char* data, size_t len) {
		if (finalized || data == 0 || len == 0)
			return;
		totalBits += (uint64_t)len * (uint64_t)8;
		absorb(data, len);
	}

	std::string hex() {
		finalize();
		static const char kHexDigits[17] = "0123456789abcdef";
		std::string out;
		out.reserve(64);
		for (int i = 0; i < 32; ++i) {
			const unsigned int b = (unsigned int)digest[i];
			out.push_back(kHexDigits[(size_t)((b >> 4) & 0xfu)]);
			out.push_back(kHexDigits[(size_t)(b & 0xfu)]);
		}
		return out;
	}

private:
	// FIPS 180-4 section 5.3.3 initial hash value.
	uint32_t h[8] = {
		0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
		0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
	};
	unsigned char buf[64] = {0};
	size_t bufLen = 0;
	uint64_t totalBits = 0;
	bool finalized = false;
	unsigned char digest[32] = {0};

	static uint32_t rotr(uint32_t x, int n) {
		return (uint32_t)((x >> n) | (x << (32 - n)));
	}

	// Buffer bytes into 64-byte blocks and compress each full block. Does NOT
	// touch totalBits, so finalize() can append padding through the same path.
	void absorb(const unsigned char* data, size_t len) {
		while (len > 0) {
			size_t take = (size_t)64 - bufLen;
			if (take > len)
				take = len;
			std::memcpy(buf + bufLen, data, take);
			bufLen += take;
			data += take;
			len -= take;
			if (bufLen == (size_t)64) {
				compress(buf);
				bufLen = 0;
			}
		}
	}

	void compress(const unsigned char* block) {
		// FIPS 180-4 section 4.2.2 — the first 32 bits of the fractional parts of
		// the cube roots of the first 64 primes. Function-local so a translation
		// unit that includes but does not use this header gets no unused-variable
		// diagnostic under -Wall -Wextra, and so no ODR question arises.
		static const uint32_t K[64] = {
			0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
			0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
			0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
			0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
			0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
			0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
			0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
			0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
			0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
			0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
			0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
			0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
			0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
			0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
			0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
			0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
		};

		uint32_t w[64];
		for (int i = 0; i < 16; ++i) {
			// Big-endian word assembly. Explicit casts on every shifted byte —
			// never rely on implicit promotion/narrowing here.
			w[i] = ((uint32_t)block[4 * i + 0] << 24)
			     | ((uint32_t)block[4 * i + 1] << 16)
			     | ((uint32_t)block[4 * i + 2] << 8)
			     | ((uint32_t)block[4 * i + 3]);
		}
		for (int i = 16; i < 64; ++i) {
			const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (uint32_t)(w[i - 15] >> 3);
			const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (uint32_t)(w[i - 2] >> 10);
			w[i] = (uint32_t)(w[i - 16] + s0 + w[i - 7] + s1);
		}

		uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
		uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

		for (int i = 0; i < 64; ++i) {
			const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
			const uint32_t ch = (uint32_t)((e & f) ^ ((~e) & g));
			const uint32_t t1 = (uint32_t)(hh + S1 + ch + K[i] + w[i]);
			const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
			const uint32_t maj = (uint32_t)((a & b) ^ (a & c) ^ (b & c));
			const uint32_t t2 = (uint32_t)(S0 + maj);
			hh = g;
			g = f;
			f = e;
			e = (uint32_t)(d + t1);
			d = c;
			c = b;
			b = a;
			a = (uint32_t)(t1 + t2);
		}

		h[0] = (uint32_t)(h[0] + a);
		h[1] = (uint32_t)(h[1] + b);
		h[2] = (uint32_t)(h[2] + c);
		h[3] = (uint32_t)(h[3] + d);
		h[4] = (uint32_t)(h[4] + e);
		h[5] = (uint32_t)(h[5] + f);
		h[6] = (uint32_t)(h[6] + g);
		h[7] = (uint32_t)(h[7] + hh);
	}

	// FIPS 180-4 section 5.1.1 padding: 0x80, then zeros, then the 64-bit
	// big-endian message length in bits. Idempotent.
	void finalize() {
		if (finalized)
			return;

		const uint64_t bits = totalBits;

		const unsigned char one = 0x80u;
		absorb(&one, 1);

		const unsigned char zero = 0x00u;
		while (bufLen != (size_t)56)
			absorb(&zero, 1);

		unsigned char lenBytes[8];
		for (int i = 0; i < 8; ++i)
			lenBytes[i] = (unsigned char)((bits >> (56 - 8 * i)) & (uint64_t)0xffu);
		absorb(lenBytes, 8);

		for (int i = 0; i < 8; ++i) {
			digest[4 * i + 0] = (unsigned char)((h[i] >> 24) & 0xffu);
			digest[4 * i + 1] = (unsigned char)((h[i] >> 16) & 0xffu);
			digest[4 * i + 2] = (unsigned char)((h[i] >> 8) & 0xffu);
			digest[4 * i + 3] = (unsigned char)(h[i] & 0xffu);
		}
		finalized = true;
	}
};

namespace detail {

// Shared file-hashing body for the two file entry points below. Streams the file
// in chunks (never loads it whole) and returns the empty string if the stream
// could not be opened, so a missing fixture is reported as a mismatch rather
// than as a silent pass.
inline std::string hashFileImpl(const std::string& path, bool stripCarriageReturns) {
	std::ifstream f(path.c_str(), std::ios::binary);
	if (!f.is_open())
		return std::string();

	Sha256 h;
	char chunk[8192];
	std::vector<unsigned char> filtered;
	while (f) {
		f.read(chunk, (std::streamsize)sizeof chunk);
		const std::streamsize n = f.gcount();
		if (n <= 0)
			break;
		if (stripCarriageReturns) {
			filtered.clear();
			filtered.reserve((size_t)n);
			for (std::streamsize i = 0; i < n; ++i) {
				const unsigned char b = (unsigned char)chunk[(size_t)i];
				if (b != (unsigned char)0x0d)
					filtered.push_back(b);
			}
			if (!filtered.empty())
				h.update(&filtered[0], filtered.size());
		}
		else {
			h.update(reinterpret_cast<const unsigned char*>(chunk), (size_t)n);
		}
	}
	return h.hex();
}

} // namespace detail

// Digest of a byte sequence.
inline std::string sha256HexBytes(const std::vector<unsigned char>& bytes) {
	Sha256 h;
	if (!bytes.empty())
		h.update(&bytes[0], bytes.size());
	return h.hex();
}

// Digest of the raw bytes of a string (no encoding conversion).
inline std::string sha256Hex(const std::string& text) {
	Sha256 h;
	if (!text.empty())
		h.update(reinterpret_cast<const unsigned char*>(text.data()), text.size());
	return h.hex();
}

// Digest of a file's bytes exactly as they sit on disk. Returns "" if the file
// cannot be opened. This is the entry point used for the binary .f32 goldens.
inline std::string sha256HexFile(const std::string& path) {
	return detail::hashFileImpl(path, false);
}

// Digest of a file with every carriage-return byte (0x0d) dropped before
// hashing. On an LF checkout the result is IDENTICAL to sha256HexFile(); the
// variant exists because GitHub's windows-latest runner checks text files out
// with CRLF and this repository ships no .gitattributes (P-3), so a raw digest
// of a text file would differ on that leg for reasons that have nothing to do
// with content. Use it for text manifests (plan 29-04); the binary .f32 goldens
// must always use the raw sha256HexFile().
inline std::string sha256HexFileLfNormalized(const std::string& path) {
	return detail::hashFileImpl(path, true);
}

} // namespace forge
