// K7b-1 (Section 62-B / Section 64) -- consteval SHA-512 Byte-Wache
//
// **COMPILE-TIME**: die NIST-/FIPS-180-4-Pruefvektoren sind via static_assert einkompiliert (leer / "abc" /
//   Fox / kanonischer 112-Byte-Zwei-Block-Vektor). Zusaetzlich RT-Pfad-Deckung (span-Ueberladung + hex-Roundtrip),
//   damit auch der Nicht-constexpr-Zweig gemessen wird.
//
// Der SHA-512-Fingerprint wird in K7b-3 zur 6. AnatomyVersionLines-Stempel-Zeile (concat(organ+system+
//   measurement+merge)); dieser Test friert das Primitiv separat ein, bevor es die POD-Naht beruehrt.

#include <gtest/gtest.h>

#include <sha512/ctsha512.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace ctsha512 = ::comdare::cache_engine::sha512;

TEST(CtSha512, EmptyStringHashCompileTime) {
    constexpr auto digest   = ctsha512::sha512("");
    constexpr auto expected = ctsha512::from_hex("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
                                                 "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
    static_assert(digest == expected, "ctsha512: SHA512(\"\") mismatch");
    SUCCEED();
}

TEST(CtSha512, AbcHashCompileTime) {
    constexpr auto digest   = ctsha512::sha512("abc");
    constexpr auto expected = ctsha512::from_hex("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                                                 "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    static_assert(digest == expected, "ctsha512: SHA512(\"abc\") mismatch");
    SUCCEED();
}

TEST(CtSha512, FoxHashCompileTime) {
    constexpr auto digest   = ctsha512::sha512("The quick brown fox jumps over the lazy dog");
    constexpr auto expected = ctsha512::from_hex("07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb64"
                                                 "2e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6");
    static_assert(digest == expected, "ctsha512: SHA512(Fox) mismatch");
    SUCCEED();
}

// Kanonischer FIPS-180-4-Zwei-Block-Vektor (896 Bit == 112 Byte): loest den Extra-Padding-Block aus
// (remaining == 112 -> 112 + 1 + 16 > 128), deckt also die Block-Grenze + die 128-Bit-Laenge.
TEST(CtSha512, TwoBlockBoundaryHashCompileTime) {
    constexpr auto digest   = ctsha512::sha512("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
                                               "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
    constexpr auto expected = ctsha512::from_hex("8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"
                                                 "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909");
    static_assert(digest == expected, "ctsha512: 112-Byte-Zwei-Block-Vektor gebrochen");
    SUCCEED();
}

TEST(CtSha512, CompileTimeBudgetCheck) {
    static_assert(ctsha512::fits_compile_time_budget<1>());
    static_assert(ctsha512::fits_compile_time_budget<50 * 1024>());
    static_assert(!ctsha512::fits_compile_time_budget<50 * 1024 + 1>());
    SUCCEED();
}

TEST(CtSha512, DigestWidthAndHexRoundTrip) {
    constexpr auto digest = ctsha512::sha512("abc");
    static_assert(digest.size() == 64u, "SHA-512 Digest == 64 Byte");
    auto const             hex = ctsha512::to_hex(digest);
    std::string_view const hv{hex.data(), hex.size()};
    EXPECT_EQ(hv.size(), 128u);
    EXPECT_EQ(hv, "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                  "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
}

// Laufzeit-Zweig: die span-Ueberladung (nicht constexpr ausgewertet) liefert dasselbe wie der consteval-Pfad.
TEST(CtSha512, RuntimeSpanPathMatchesConsteval) {
    std::array<std::uint8_t, 3> const msg{{std::uint8_t{'a'}, std::uint8_t{'b'}, std::uint8_t{'c'}}};
    auto const                        digest   = ctsha512::sha512(std::span<const std::uint8_t>{msg});
    constexpr auto                    expected = ctsha512::sha512("abc");
    EXPECT_TRUE(std::equal(digest.begin(), digest.end(), expected.begin()));
}
