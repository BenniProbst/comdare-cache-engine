#pragma once
// K7b-1 (Section 62-B / Section 64) -- consteval SHA-512 fuer den Anatomy-Stempel-Fingerprint
//
// @stand K7b-1 (2026-07-22, POD-Stempel-Umbau: SHA512-Fingerprint-Zeile als 6. AnatomyVersionLines-Feld)
// @reference RFC 6234 (Eastlake/Hansen 2011, US Secure Hash Algorithms) / FIPS 180-4
//
// Standalone-Implementation (kein externes Lib, keine Krypto-Abhaengigkeit, kein Python). Spiegelt die
// bestehende consteval SHA-256 (src/sha256/ctsha.hpp, inspiriert von vexingcodes/ctsha, MIT-Lizenz) auf die
// SHA-512-Familie: 64-Bit-Worte, 80 Runden, 128-Byte-Bloecke, 128-Bit-Laengenfeld, 64-Byte-Digest. Berechnet
// zur Compile-Zeit (consteval/constexpr) ueber std::span<std::uint8_t const> bzw. char[N]. Praktikabel bis
// ~50KB pro Aufruf (Compile-Time-Budget, wie SHA-256).
//
// Verwendung:
//   constexpr auto digest = comdare::cache_engine::sha512::sha512("achse=algo@1.0.0;...");
//   static_assert(digest == comdare::cache_engine::sha512::from_hex("cf83e1..."));
//
// Die NIST-Pruefvektoren (leerer String / "abc" / Fox) sind unten als static_assert einkompiliert -> ein
// falsch abgeschriebener Konstanten- oder Rotations-Wert bricht bereits das Uebersetzen dieses Headers.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::sha512 {

using Digest = std::array<std::uint8_t, 64>;

namespace detail {

/// SHA-512-Rundenkonstanten: die ersten 64 Bit der Nachkommastellen der Kubikwurzeln der ersten 80 Primzahlen.
inline constexpr std::array<std::uint64_t, 80> kK{{
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL,
    0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL, 0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
    0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL, 0x983e5152ee66dfabULL,
    0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL,
    0x53380d139d95b3dfULL, 0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
    0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL, 0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL,
    0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL, 0xca273eceea26619cULL,
    0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL, 0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
    0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
}};

constexpr std::uint64_t rotr(std::uint64_t x, std::uint32_t n) noexcept { return (x >> n) | (x << (64 - n)); }

constexpr void process_block(std::array<std::uint64_t, 8>& h, std::span<const std::uint8_t, 128> block) noexcept {
    std::array<std::uint64_t, 80> w{};
    for (std::size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint64_t>(block[8 * i + 0]) << 56) |
               (static_cast<std::uint64_t>(block[8 * i + 1]) << 48) |
               (static_cast<std::uint64_t>(block[8 * i + 2]) << 40) |
               (static_cast<std::uint64_t>(block[8 * i + 3]) << 32) |
               (static_cast<std::uint64_t>(block[8 * i + 4]) << 24) |
               (static_cast<std::uint64_t>(block[8 * i + 5]) << 16) |
               (static_cast<std::uint64_t>(block[8 * i + 6]) << 8) | static_cast<std::uint64_t>(block[8 * i + 7]);
    }
    for (std::size_t i = 16; i < 80; ++i) {
        std::uint64_t s0 = rotr(w[i - 15], 1) ^ rotr(w[i - 15], 8) ^ (w[i - 15] >> 7);
        std::uint64_t s1 = rotr(w[i - 2], 19) ^ rotr(w[i - 2], 61) ^ (w[i - 2] >> 6);
        w[i]             = w[i - 16] + s0 + w[i - 7] + s1;
    }
    std::uint64_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint64_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (std::size_t i = 0; i < 80; ++i) {
        std::uint64_t S1 = rotr(e, 14) ^ rotr(e, 18) ^ rotr(e, 41);
        std::uint64_t ch = (e & f) ^ (~e & g);
        std::uint64_t t1 = hh + S1 + ch + kK[i] + w[i];
        std::uint64_t S0 = rotr(a, 28) ^ rotr(a, 34) ^ rotr(a, 39);
        std::uint64_t mj = (a & b) ^ (a & c) ^ (b & c);
        std::uint64_t t2 = S0 + mj;
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

/// Haupt-Implementation: arbeitet direkt auf raw pointer + size (MSVC-tauglich, wie sha256_bytes).
constexpr Digest sha512_bytes(const std::uint8_t* data, std::size_t len) noexcept {
    std::array<std::uint64_t, 8> h{{
        0x6a09e667f3bcc908ULL,
        0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL,
        0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,
        0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL,
        0x5be0cd19137e2179ULL,
    }};

    std::uint64_t                 total_bytes = len;
    std::array<std::uint8_t, 128> block{};
    std::size_t                   pos = 0;

    while (len - pos >= 128) {
        for (std::size_t i = 0; i < 128; ++i) block[i] = data[pos + i];
        process_block(h, std::span<const std::uint8_t, 128>{block});
        pos += 128;
    }

    std::size_t remaining = len - pos;
    for (std::size_t i = 0; i < remaining; ++i) block[i] = data[pos + i];
    block[remaining] = 0x80;
    for (std::size_t i = remaining + 1; i < 128; ++i) block[i] = 0;

    // Das Laengenfeld ist 128 Bit (16 Byte). Reicht der Rest fuer 0x80 + 16 Byte nicht, folgt ein Extra-Block.
    if (remaining + 1 + 16 > 128) {
        process_block(h, std::span<const std::uint8_t, 128>{block});
        for (auto& b : block) b = 0;
    }

    // 128-Bit-Laenge big-endian: die oberen 64 Bit bleiben 0 (Byte 112..119), die unteren 64 Bit ans Blockende.
    std::uint64_t bit_length = total_bytes * 8;
    for (std::size_t i = 0; i < 8; ++i) { block[127 - i] = static_cast<std::uint8_t>((bit_length >> (8 * i)) & 0xff); }
    process_block(h, std::span<const std::uint8_t, 128>{block});

    Digest digest{};
    for (std::size_t i = 0; i < 8; ++i) {
        for (std::size_t j = 0; j < 8; ++j) {
            digest[8 * i + j] = static_cast<std::uint8_t>((h[i] >> (56 - 8 * j)) & 0xff);
        }
    }
    return digest;
}

} // namespace detail

/// SHA-512 ueber std::span<const std::uint8_t> -- consteval-faehig.
constexpr Digest sha512(std::span<const std::uint8_t> data) noexcept {
    return detail::sha512_bytes(data.data(), data.size());
}

/// Konvenienz: Hash ueber const char* + N (constexpr, MSVC-tauglich; N-1 == Laenge ohne '\0').
template <std::size_t N>
constexpr Digest sha512(const char (&str)[N]) noexcept {
    constexpr std::size_t          kLen = N - 1;
    std::array<std::uint8_t, kLen> bytes{};
    for (std::size_t i = 0; i < kLen; ++i) { bytes[i] = static_cast<std::uint8_t>(str[i]); }
    return detail::sha512_bytes(bytes.data(), bytes.size());
}

/// Compile-Time-Budget-Check: Source-Size bleibt unter Grenze (identische Grenze wie SHA-256).
template <std::size_t N>
consteval bool fits_compile_time_budget() noexcept {
    constexpr std::size_t kMaxFunctionBodyBytes = 50 * 1024; // 50 KB
    return N <= kMaxFunctionBodyBytes;
}

/// consteval Konvertierung 128-char hex string -> Digest (64 Byte). Defekte Laenge -> all-zero Marker.
consteval Digest from_hex(std::string_view hex) noexcept {
    Digest d{};
    if (hex.size() != 128) return d;
    auto to_nibble = [](char c) -> std::uint8_t {
        if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
        return 0;
    };
    for (std::size_t i = 0; i < 64; ++i) {
        d[i] = static_cast<std::uint8_t>((to_nibble(hex[2 * i]) << 4) | to_nibble(hex[2 * i + 1]));
    }
    return d;
}

/// constexpr Konvertierung Digest -> 128-char hex string (fuer die Stempel-Zeile + Diagnose).
constexpr std::array<char, 128> to_hex(Digest const& d) noexcept {
    std::array<char, 128> out{};
    constexpr char        kHex[] = "0123456789abcdef";
    for (std::size_t i = 0; i < 64; ++i) {
        out[2 * i + 0] = kHex[(d[i] >> 4) & 0x0f];
        out[2 * i + 1] = kHex[d[i] & 0x0f];
    }
    return out;
}

// NIST-/FIPS-180-4-Pruefvektoren, einkompiliert: bricht das Uebersetzen bei falschen Konstanten/Rotationen.
static_assert(sha512("") == from_hex("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
                                     "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"),
              "SHA-512(\"\") Pruefvektor gebrochen");
static_assert(sha512("abc") == from_hex("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                                        "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"),
              "SHA-512(\"abc\") Pruefvektor gebrochen");
static_assert(sha512("The quick brown fox jumps over the lazy dog") ==
                  from_hex("07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb64"
                           "2e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6"),
              "SHA-512(Fox) Pruefvektor gebrochen");

} // namespace comdare::cache_engine::sha512
