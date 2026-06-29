// V41.A5 (2026-05-24) - Unit-Tests fuer echte Algorithmus-Implementierungen

#include <cache_engine/indexes/linear_probe_hashset.hpp>
#include <cache_engine/indexes/radix_index.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <vector>

namespace ix = comdare::cache_engine::indexes;

TEST(LinearProbeHashSet, BasicInsertContains) {
    ix::LinearProbeHashSet<std::uint64_t> h;
    EXPECT_FALSE(h.contains(42));
    EXPECT_TRUE(h.insert(42));
    EXPECT_TRUE(h.contains(42));
    EXPECT_FALSE(h.insert(42)); // duplicate
    EXPECT_EQ(h.size(), 1u);
}

TEST(LinearProbeHashSet, ZeroKeyIsStorable) {
    // Regression (2026-06-01): der als Empty-Marker reservierte Wert 0 muss
    // speicher- UND auffindbar sein. Vorher wurde 0 still verworfen, wodurch
    // scalar-Permutationen mit mix(0)=0*kPrime=0 systematisch 1 Treffer verloren
    // (run()=-2). 0 wird jetzt separat via has_zero_ gefuehrt.
    ix::LinearProbeHashSet<std::uint64_t> h;
    EXPECT_FALSE(h.contains(0)); // leer: nicht enthalten
    EXPECT_TRUE(h.insert(0));    // 0 ist einfuegbar
    EXPECT_TRUE(h.contains(0));  // und auffindbar
    EXPECT_FALSE(h.insert(0));   // Duplikat
    EXPECT_EQ(h.size(), 1u);
    // Gemischt mit Nicht-Null-Key: loest den lazy reserve(16) im Nicht-Null-Pfad aus;
    // has_zero_ darf dabei NICHT verlorengehen.
    EXPECT_TRUE(h.insert(7));
    EXPECT_TRUE(h.contains(0));
    EXPECT_TRUE(h.contains(7));
    EXPECT_EQ(h.size(), 2u);
}

TEST(LinearProbeHashSet, ZeroKeySurvivesGrow) {
    // 0 muss auch ueber grow() (Rehash) erhalten bleiben und in size() zaehlen.
    ix::LinearProbeHashSet<std::uint64_t> h(16);
    EXPECT_TRUE(h.insert(0));
    constexpr std::size_t kN = 500; // erzwingt mehrere grow()
    for (std::size_t i = 1; i <= kN; ++i) { EXPECT_TRUE(h.insert(static_cast<std::uint64_t>(i * 17))); }
    EXPECT_TRUE(h.contains(0));   // ueber grow() hinweg auffindbar
    EXPECT_EQ(h.size(), kN + 1u); // +1 fuer den Sentinel-Eintrag
    for (std::size_t i = 1; i <= kN; ++i) { EXPECT_TRUE(h.contains(static_cast<std::uint64_t>(i * 17))); }
}

TEST(LinearProbeHashSet, GrowsBeyondInitialCapacity) {
    ix::LinearProbeHashSet<std::uint64_t> h(16);
    constexpr std::size_t                 kN = 500;
    for (std::size_t i = 1; i <= kN; ++i) { EXPECT_TRUE(h.insert(static_cast<std::uint64_t>(i * 17))); }
    EXPECT_EQ(h.size(), kN);
    for (std::size_t i = 1; i <= kN; ++i) { EXPECT_TRUE(h.contains(static_cast<std::uint64_t>(i * 17))); }
    EXPECT_FALSE(h.contains(0));        // sentinel
    EXPECT_FALSE(h.contains(99999999)); // not inserted
}

TEST(LinearProbeHashSet, NoFalsePositivesRandom) {
    ix::LinearProbeHashSet<std::uint64_t> h;
    std::mt19937_64                       rng{0xC0FFEE};
    std::vector<std::uint64_t>            inserted;
    inserted.reserve(200);
    for (int i = 0; i < 200; ++i) {
        std::uint64_t k = rng();
        if (k == 0) k = 1;
        h.insert(k);
        inserted.push_back(k);
    }
    for (auto k : inserted) { EXPECT_TRUE(h.contains(k)); }
    // Random keys not in set
    std::size_t false_positive = 0;
    for (int i = 0; i < 500; ++i) {
        std::uint64_t k = rng() | 0xDEAD'BEEF'0000'0000ULL;
        if (h.contains(k)) ++false_positive;
    }
    EXPECT_LT(false_positive, 5u);
}

TEST(RadixIndex, BasicInsertContains) {
    ix::RadixIndex r;
    EXPECT_FALSE(r.contains(123));
    r.insert(123);
    EXPECT_TRUE(r.contains(123));
    r.insert(123); // duplicate
    EXPECT_EQ(r.size(), 1u);
}

TEST(RadixIndex, ManyKeysSizeMatches) {
    ix::RadixIndex r;
    for (std::uint32_t i = 1; i <= 1000; ++i) { r.insert(i * 7); }
    EXPECT_EQ(r.size(), 1000u);
    for (std::uint32_t i = 1; i <= 1000; ++i) { EXPECT_TRUE(r.contains(i * 7)); }
    EXPECT_FALSE(r.contains(0));
    EXPECT_FALSE(r.contains(8)); // 1*7=7, 2*7=14, 8 ist dazwischen
}
