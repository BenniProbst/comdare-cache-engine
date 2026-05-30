// V41 SuRF S2 Inkrement 1 (Task #42-Folge) — echtes succinct LOUDS-Sparse-Filter-Organ: Property-Tests.
//
// S2 (ComposedSurfLoudsFilter) ist der ECHTE approximative succinct Range-Filter (FP>0, tunbar), Gegenstueck
// zur exakten S1-Base (ComposedExactSurfFilter, FP=0). Beweispflicht:
//   P1 NO-FALSE-NEGATIVE (HART): jeder gebaute Key wird gefunden — ueber kNone/kReal8/kReal16.
//   KREUZBELEG: S2.contains(k) >= S1.contains(k) ueber den gesamten Probe-Raum (no-FN gegen exakte Ground-Truth).
//   TUNBARKEIT: FP-Rate monoton fallend UND bits_per_key streng steigend ueber RealLen {0,4,8,16}.
//   P3 RANGE-NO-FN: jeder Range mit >=1 Key liefert true (gegen std::set).
//   ADVERSARIAL: Byte-Praefix-Ketten + Singleton + leerer Trie + clear().

#include <gtest/gtest.h>

#include <topics/filter/axis_filter/composable/louds_sparse_filter_organ.hpp>
#include <topics/filter/axis_filter/composable/exact_prefix_filter_organ.hpp>

#include <cstdint>
#include <set>
#include <vector>

namespace fcmp = ::comdare::cache_engine::filter::axis_filter::composable;

using S1     = fcmp::ComposedExactSurfFilter<fcmp::ExactPrefixFilterQuery, fcmp::ExactPrefixFilterStore>;
using S2None = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kNone, 0, 0>;
using S2R4   = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kReal, 0, 4>;
using S2R8   = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kReal, 0, 8>;
using S2R16  = fcmp::ComposedSurfLoudsFilter<fcmp::SurfSuffixType::kReal, 0, 16>;

namespace {
// Verstreute uint64-Keys (wie S1-Test): robust fuer no-FN/Kreuzbeleg/Range.
std::vector<std::uint64_t> build_key_set(std::set<std::uint64_t>& gt) {
    for (std::uint64_t i = 0; i < 800; ++i) gt.insert((i * 2654435761u) % 100000u);
    return std::vector<std::uint64_t>(gt.begin(), gt.end());
}
// Flach-Leaf-Keys: Byte 5 (=i) diskriminiert => Blatt bei Level 5; Bytes 6/7 tragen NICHT-NULL variierende
// Suffix-Daten => echte Suffix-Diskriminierung (sonst waere der Real-Suffix konstant 0, kein FP-Tuning).
std::vector<std::uint64_t> build_shallow_keys(std::set<std::uint64_t>& gt) {
    for (std::uint64_t i = 1; i <= 255; ++i) gt.insert((i << 16) | ((i * 2654435761u) & 0xFFFFu));
    return std::vector<std::uint64_t>(gt.begin(), gt.end());
}

template <class Filter>
[[nodiscard]] bool all_built_found(std::vector<std::uint64_t> const& keys, std::set<std::uint64_t> const& gt) {
    Filter f; f.build_from_sorted_keys(keys);
    if (f.key_count() != gt.size()) return false;
    for (std::uint64_t const k : gt) if (!f.contains(k)) return false;
    return true;
}
}  // namespace

// P1: NO-FALSE-NEGATIVE — jeder gebaute Key MUSS gefunden werden, fuer alle Suffix-Konfigurationen.
TEST(SurfLoudsFilter, NoFalseNegativeAcrossSuffixConfigs) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    EXPECT_TRUE(all_built_found<S2None>(keys, gt)) << "kNone no-FN";
    EXPECT_TRUE(all_built_found<S2R8>(keys, gt))   << "kReal8 no-FN";
    EXPECT_TRUE(all_built_found<S2R16>(keys, gt))  << "kReal16 no-FN";
}

// KREUZBELEG: S2.contains(q) >= S1.contains(q) fuer JEDES q — wo S1 (exakt) true sagt, MUSS S2 true sagen.
TEST(SurfLoudsFilter, CrossProofS2GeS1) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    S1 s1;  s1.build_from_sorted_keys(keys);
    S2R8 s2; s2.build_from_sorted_keys(keys);
    for (std::uint64_t q = 0; q <= 100000u; ++q) {
        if (s1.contains(q)) ASSERT_TRUE(s2.contains(q)) << "S2 FALSE NEGATIVE bei q=" << q;
    }
}

// TUNBARKEIT: FP-Rate monoton fallend + bits_per_key streng steigend ueber RealLen {0,4,8,16}.
TEST(SurfLoudsFilter, FalsePositiveMonotoneTunable) {
    std::set<std::uint64_t> gt;
    auto const keys = build_shallow_keys(gt);

    auto measure = [&](auto filter) {
        filter.build_from_sorted_keys(keys);
        std::size_t fp = 0;
        for (std::uint64_t i = 1; i <= 255; ++i)
            for (std::uint64_t low = 0; low <= 4000u; ++low) {
                std::uint64_t const q = (i << 16) | low;
                if (gt.count(q) == 0u && filter.contains(q)) ++fp;
            }
        return std::pair<std::size_t, double>{fp, filter.bits_per_key()};
    };

    auto [fp0,  bpk0]  = measure(S2R4{});   // 4-Bit-Real
    auto [fp_a, bpk_a] = measure(S2R8{});
    auto [fp_b, bpk_b] = measure(S2R16{});
    auto [fpN,  bpkN]  = measure(S2None{}); // 0-Bit (Trie-only, Max-FP-Anker)

    // bits_per_key streng steigend mit Suffix-Laenge (jedes Blatt speichert suffix_len Bits).
    EXPECT_LT(bpkN, bpk0); EXPECT_LT(bpk0, bpk_a); EXPECT_LT(bpk_a, bpk_b);
    // FP monoton fallend mit Suffix-Laenge.
    EXPECT_GE(fpN, fp0); EXPECT_GE(fp0, fp_a); EXPECT_GE(fp_a, fp_b);
    // Es entsteht ueberhaupt FP (S2 ist der approximative Filter) und laengster Suffix diskriminiert echt besser.
    EXPECT_GT(fpN, 0u);
    EXPECT_LT(fp_b, fpN) << "kReal16 muss strikt weniger FP haben als der Trie-only-Anker";
}

// P3: RANGE-NO-FALSE-NEGATIVE — jeder Range mit >=1 gebautem Key MUSS true liefern (FP erlaubt).
TEST(SurfLoudsFilter, RangeNoFalseNegativeAgainstStdSet) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    S2R8 s2; s2.build_from_sorted_keys(keys);
    for (std::uint64_t lo = 0; lo <= 2000u; lo += 7u) {
        for (std::uint64_t w : {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{50}, std::uint64_t{500}}) {
            std::uint64_t const hi = lo + w;
            auto it = gt.lower_bound(lo);
            bool const truth = (it != gt.end() && *it <= hi);
            if (truth) ASSERT_TRUE(s2.range_may_exist(lo, hi)) << "RANGE FALSE NEGATIVE [" << lo << "," << hi << "]";
        }
    }
}

// ADVERSARIAL: Byte-Praefix-Ketten + Singleton + leerer Trie + clear().
TEST(SurfLoudsFilter, AdversarialPrefixSingletonEmpty) {
    // Byte-Praefix-Kette (ein Key ist Byte-Praefix-verwandt mit anderen).
    std::vector<std::uint64_t> chain = {0x0000000000000001ull, 0x0000000000000100ull,
                                        0x0000000000010000ull, 0x0000000001000000ull,
                                        0x0000000100000000ull};
    {
        S2R8 f; f.build_from_sorted_keys(chain);
        for (std::uint64_t k : chain) EXPECT_TRUE(f.contains(k)) << "Praefix-Ketten-Key " << k;
    }
    // Singleton.
    {
        std::vector<std::uint64_t> one = {0x00DEADBEEF00CAFEull};
        S2R16 f; f.build_from_sorted_keys(one);
        EXPECT_EQ(f.key_count(), 1u);
        EXPECT_TRUE(f.contains(0x00DEADBEEF00CAFEull));
    }
    // Leerer Trie.
    {
        std::vector<std::uint64_t> none;
        S2R8 f; f.build_from_sorted_keys(none);
        EXPECT_EQ(f.key_count(), 0u);
        EXPECT_FALSE(f.contains(0));
        EXPECT_FALSE(f.range_may_exist(0, ~0ull));
        EXPECT_DOUBLE_EQ(f.bits_per_key(), 0.0);
    }
    // clear() -> wieder leer.
    {
        std::set<std::uint64_t> gt;
        auto const keys = build_key_set(gt);
        S2R8 f; f.build_from_sorted_keys(keys);
        f.clear();
        EXPECT_EQ(f.key_count(), 0u);
        EXPECT_FALSE(f.contains(*gt.begin()));
    }
}

// REGRESSION (Verifikation wuegyse1h): Keyset, das den louds-Bitvektor auf exakt 64 Bits (Wort-Grenze)
// mit Carry bringt -> vormals heap-buffer-overflow in SurfBitVector::concatenate (One-Past-End-Write).
// Vor dem +1-Carry-Wort-Fix CRASH; danach: Build ok + no-FN.
TEST(SurfLoudsFilter, WordBoundaryCarryNoOverflow) {
    std::vector<std::uint64_t> keys = {0xFFFFFFFFFFFFFFAAull, 0xFFFFFFFFFFFFFFBBull};
    for (std::uint64_t i = 0; i < 48; ++i) keys.push_back((i * 0x0100000000000001ull) & 0x00FFFFFFFFFFFFFFull);
    std::set<std::uint64_t> gt(keys.begin(), keys.end());
    std::vector<std::uint64_t> sorted(gt.begin(), gt.end());
    S2R8 f; f.build_from_sorted_keys(sorted);   // darf NICHT crashen
    EXPECT_EQ(f.key_count(), gt.size());
    for (std::uint64_t const k : gt) EXPECT_TRUE(f.contains(k)) << "no-FN bei Wort-Grenz-Key " << k;
}

// Observer: bit_size>0, bits_per_key approximativ (deutlich < 64 = S1-exakt) und > 0.
TEST(SurfLoudsFilter, ObserversApproximateVsExact) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    S2R8 s2; s2.build_from_sorted_keys(keys);
    EXPECT_GT(s2.bit_size(), 0u);
    EXPECT_GT(s2.bits_per_key(), 0.0);
    EXPECT_FALSE(S2R8::is_original_module());
}
