#pragma once
// V41.F.6.1.P1 Phase B — consteval SHA256 fuer Paper-Original-Code-Validierung
//
// @stand V41.F.6.1.P1 Phase B Pilot mimalloc
// @reference RFC 6234 (Eastlake/Hansen 2011, US Secure Hash Algorithms)
// @reference [[consteval-sha256-function-validation]] Memory
//
// Standalone-Implementation (kein externes Lib). Inspiriert von vexingcodes/ctsha
// (MIT-Lizenz). Berechnet SHA-256 zur Compile-Zeit (consteval/constexpr) ueber
// std::span<std::byte const> Input. Praktikabel bis ~50KB pro Aufruf (User-Direktive
// [[legacy-code-sha256-validation]] Compile-Time-Budget-Check).
//
// Verwendung:
//   constexpr auto digest = comdare::sha256(std::span{src_bytes});
//   static_assert(digest == kExpectedDigest);

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::sha256 {

using Digest = std::array<std::uint8_t, 32>;

namespace detail {

inline constexpr std::array<std::uint32_t, 64> kK{{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
}};

constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n) noexcept { return (x >> n) | (x << (32 - n)); }

constexpr void process_block(std::array<std::uint32_t, 8>& h, std::span<const std::uint8_t, 64> block) noexcept {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[4 * i + 0]) << 24) |
               (static_cast<std::uint32_t>(block[4 * i + 1]) << 16) |
               (static_cast<std::uint32_t>(block[4 * i + 2]) << 8) | static_cast<std::uint32_t>(block[4 * i + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
        std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i]             = w[i - 16] + s0 + w[i - 7] + s1;
    }
    std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (std::size_t i = 0; i < 64; ++i) {
        std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        std::uint32_t ch = (e & f) ^ (~e & g);
        std::uint32_t t1 = hh + S1 + ch + kK[i] + w[i];
        std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        std::uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        std::uint32_t t2 = S0 + mj;
        hh               = g;
        g                = f;
        f                = e;
        e                = d + t1;
        d                = c;
        c                = b;
        b                = a;
        a                = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
}

} // namespace detail

namespace detail {

/// Haupt-Implementation: arbeitet direkt auf raw pointer + size (MSVC-tauglich)
constexpr Digest sha256_bytes(const std::uint8_t* data, std::size_t len) noexcept {
    std::array<std::uint32_t, 8> h{{
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19,
    }};

    std::uint64_t                total_bytes = len;
    std::array<std::uint8_t, 64> block{};
    std::size_t                  pos = 0;

    while (len - pos >= 64) {
        for (std::size_t i = 0; i < 64; ++i) block[i] = data[pos + i];
        process_block(h, std::span<const std::uint8_t, 64>{block});
        pos += 64;
    }

    std::size_t remaining = len - pos;
    for (std::size_t i = 0; i < remaining; ++i) block[i] = data[pos + i];
    block[remaining] = 0x80;
    for (std::size_t i = remaining + 1; i < 64; ++i) block[i] = 0;

    if (remaining + 1 + 8 > 64) {
        process_block(h, std::span<const std::uint8_t, 64>{block});
        for (auto& b : block) b = 0;
    }

    std::uint64_t bit_length = total_bytes * 8;
    for (std::size_t i = 0; i < 8; ++i) { block[63 - i] = static_cast<std::uint8_t>((bit_length >> (8 * i)) & 0xff); }
    process_block(h, std::span<const std::uint8_t, 64>{block});

    Digest digest{};
    for (std::size_t i = 0; i < 8; ++i) {
        digest[4 * i + 0] = static_cast<std::uint8_t>((h[i] >> 24) & 0xff);
        digest[4 * i + 1] = static_cast<std::uint8_t>((h[i] >> 16) & 0xff);
        digest[4 * i + 2] = static_cast<std::uint8_t>((h[i] >> 8) & 0xff);
        digest[4 * i + 3] = static_cast<std::uint8_t>(h[i] & 0xff);
    }
    return digest;
}

} // namespace detail

/**
 * @brief SHA-256 Hash ueber std::span<const std::uint8_t> — consteval-faehig
 *
 * @note Praktikabel bis ~50KB Input (Compile-Time-Budget User-Direktive
 *       [[legacy-code-sha256-validation]]).
 */
constexpr Digest sha256(std::span<const std::uint8_t> data) noexcept {
    return detail::sha256_bytes(data.data(), data.size());
}

/// Konvenienz: Hash ueber const char* + N (constexpr, MSVC-tauglich)
template <std::size_t N>
constexpr Digest sha256(const char (&str)[N]) noexcept {
    constexpr std::size_t          kLen = N - 1;
    std::array<std::uint8_t, kLen> bytes{};
    for (std::size_t i = 0; i < kLen; ++i) { bytes[i] = static_cast<std::uint8_t>(str[i]); }
    return detail::sha256_bytes(bytes.data(), bytes.size());
}

/// Compile-Time-Budget-Check: Source-Size bleibt unter Grenze.
template <std::size_t N>
consteval bool fits_compile_time_budget() noexcept {
    constexpr std::size_t kMaxFunctionBodyBytes = 50 * 1024; // 50 KB
    return N <= kMaxFunctionBodyBytes;
}

/// consteval Konvertierung 64-char hex string → Digest
consteval Digest from_hex(std::string_view hex) noexcept {
    Digest d{};
    if (hex.size() != 64) return d; // Defekt-Marker: bleibt all-zero
    auto to_nibble = [](char c) -> std::uint8_t {
        if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
        return 0;
    };
    for (std::size_t i = 0; i < 32; ++i) {
        d[i] = static_cast<std::uint8_t>((to_nibble(hex[2 * i]) << 4) | to_nibble(hex[2 * i + 1]));
    }
    return d;
}

/// constexpr Konvertierung Digest → 64-char hex string (fuer Diagnose)
constexpr std::array<char, 64> to_hex(Digest const& d) noexcept {
    std::array<char, 64> out{};
    constexpr char       kHex[] = "0123456789abcdef";
    for (std::size_t i = 0; i < 32; ++i) {
        out[2 * i + 0] = kHex[(d[i] >> 4) & 0x0f];
        out[2 * i + 1] = kHex[d[i] & 0x0f];
    }
    return out;
}

} // namespace comdare::cache_engine::sha256

// Include from_hex / to_hex auch im namespace verfuegbar machen
#include <string_view>
