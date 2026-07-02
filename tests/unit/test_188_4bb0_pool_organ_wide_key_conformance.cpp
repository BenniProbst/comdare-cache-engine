// #188-4b-b0 (2026-07-01) — Verhaltens-Konformität der 9 Weg-B-Pool-Organe gegen std::map<uint64,uint64>
// über einen WEITEN Schlüsselraum (inkl. Keys >= 2^16). GTest im Haupt-Suite (voller Organ-Include-Stack:
// libs/cache_engine + include + src + generated; die Organe ziehen path_compression-Flags + measurement-Concepts).
//
// **Warum (Explore+Codex konvergent, Prerequisite für 4b-b1):** 4b-b1 stellt in `abi_adapter.hpp` für die 9
// Pool-Familien `container_` von einem flachen SortedBinary-Spiegel auf `ObservableComposedContainer<
// organ_for_search_algo_t<SearchAlgo>>` (das NATIVE u64-Organ) um und routet T0/lookup/insert/erase darüber.
// Zwei Befunde machen diesen Test unverzichtbar VOR dem Eingriff:
//   1. Der Produktions-Konformitäts-Gate (`builder/pruef_dock/conformance_gate.hpp`) ist TRUNCATION-BLIND —
//      er testet nur Keys < 2^16 (rng()%256, 7/999/42). Er würde weder die u8/u16-Truncation der heutigen
//      Monolith-Wrapper (search_organ_) noch deren Fix durch das u64-Organ je auslösen.
//   2. Die 9 Pool-Organe (BstTreeOrgan, HashSearchOrgan, …) werden von KEINER Reference-Composition benutzt
//      und sind daher VERHALTENS-mäßig nie gegen std::map gate-getestet (nur Typ-static_asserts in
//      organ_for_search_algo.hpp). Der Swap darf ein bislang grünes (weil truncation-blindes) Binary NICHT
//      auf ein ungetestetes Organ umlenken.
//
// Dieser Test belegt für JEDES der 9 Organe (genau in der Form `ObservableComposedContainer<Organ>`, die
// `container_` in 4b-b1 annimmt): (a) key_type == uint64 (kein Truncation-Typ, K9-d-Fix, static_assert),
// (b) insert/lookup/erase/update/occupied_count/clear-Semantik bit-identisch zu std::map über Keys 0 …
// UINT64_MAX, (c) explizite Truncation-Probe: Keys 0 und 65536 (die ein u16-Substrat aliasen würde) distinkt.
//
// Additiv/gate-frei: berührt WEDER den abi_adapter NOCH den Produktions-Konformitäts-Gate (dessen Erweiterung
// auf weite Keys würde die heutigen u16-Monolith-Binaries brechen — daher hier organ-direkt, nicht am Gate).

#include <axes/lookup/composable/organ_for_search_algo.hpp>         // organ_for_search_algo_t + 9 Wrapper-Fwd + Organe
#include <axes/lookup/composable/observable_composed_container.hpp> // ObservableComposedContainer<Organ>

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <type_traits>
#include <vector>

namespace {

namespace lk  = ::comdare::cache_engine::lookup;
namespace lkc = ::comdare::cache_engine::lookup::composable;

// Weiter Schlüsselraum — der Kern: Keys >= 2^16 (die ein u16-Substrat truncatet) bis UINT64_MAX. Alle distinkt.
std::vector<std::uint64_t> wide_key_set() {
    return {0u,
            1u,
            7u,
            42u,
            255u,
            256u,
            999u,
            65535u,
            65536u,
            65537u, // 65536/65537: u16-Truncations-Schwelle
            131072u,
            (1ull << 20),
            (1ull << 32),
            (1ull << 33), // 2^32 und 2^33 (distinkt)
            (1ull << 40),
            (1ull << 48),
            0x00000000FFFFFFFFull,
            0xFFFFFFFFFFFF0000ull,
            0xDEADBEEFCAFEBABEull,
            0xFFFFFFFFFFFFFFFFull};
}

// Verifiziert EIN Pool-Organ in der 4b-b1-container_-Form gegen std::map über den weiten Key-Raum.
template <class Wrapper>
void verify_pool_organ_wide_key_conformance(char const* name) {
    using Organ = lkc::organ_for_search_algo_t<Wrapper>;
    static_assert(!std::is_same_v<Organ, void>,
                  "verify_pool_organ_wide_key_conformance nur für gemappte Weg-B-Pool-Familien (organ != void)");
    using Container = lkc::ObservableComposedContainer<Organ>;
    // (a) Typ-Beleg: das Organ trägt u64 — der Kern des K9-d-Fixes gegenüber dem u8/u16-Monolith-Wrapper.
    static_assert(std::is_same_v<typename Container::key_type, std::uint64_t>,
                  "Pool-Organ MUSS uint64-Key tragen (kein Truncation-Substrat) — K9-d-Fix");
    using V = typename Container::value_type;

    Container                              c;
    std::map<std::uint64_t, std::uint64_t> oracle;
    auto const                             keys = wide_key_set();
    auto const val_for                 = [](std::uint64_t k) -> std::uint64_t { return k ^ 0x9E3779B97F4A7C15ull; };
    auto const expect_for_each_matches = [&](char const* phase) {
        std::map<std::uint64_t, std::uint64_t> harvested;
        std::size_t const                      visits = c.for_each_record([&](std::uint64_t k, std::uint64_t v) {
            bool const inserted = harvested.emplace(k, v).second;
            EXPECT_TRUE(inserted) << name << ": for_each_record Duplicate key=" << k << " phase=" << phase;
        });
        EXPECT_EQ(visits, oracle.size()) << name << ": for_each_record Rueckgabe phase=" << phase;
        EXPECT_EQ(visits, c.occupied_count()) << name << ": for_each_record vs occupied_count phase=" << phase;
        EXPECT_EQ(harvested, oracle) << name << ": for_each_record Paare phase=" << phase;
    };

    // (b1) insert je Key: neu-Flag + occupied_count bit-identisch zu std::map. Eine u16-Truncation würde 0 und
    // 65536 kollabieren → occupied_count < oracle.size() → hier gefangen.
    for (std::uint64_t k : keys) {
        bool const org_new = c.insert(k, static_cast<V>(val_for(k)));
        bool const map_new = oracle.insert_or_assign(k, val_for(k)).second;
        EXPECT_EQ(org_new, map_new) << name << ": insert neu-Flag key=" << k;
        EXPECT_EQ(c.occupied_count(), oracle.size()) << name << ": occupied_count nach insert key=" << k;
    }

    // (b1b) for_each_record: Key-Ernte aus dem realen Pool-Organ, exakt gleiche Paare wie std::map.
    expect_for_each_matches("nach insert");

    // (b2) lookup jeder eingefügte Key = Treffer mit exaktem Wert.
    for (std::uint64_t k : keys) {
        auto const ov = c.lookup(k);
        ASSERT_TRUE(ov.has_value()) << name << ": lookup-Treffer erwartet key=" << k;
        EXPECT_EQ(static_cast<std::uint64_t>(*ov), oracle.at(k)) << name << ": Wert key=" << k;
    }

    // (c) EXPLIZITE Truncation-Probe: 0 und 65536 (u16-Aliase) müssen distinkte Werte tragen.
    {
        auto const v0     = c.lookup(0u);
        auto const v65536 = c.lookup(65536u);
        ASSERT_TRUE(v0.has_value() && v65536.has_value()) << name << ": Truncation-Probe-Keys fehlen";
        EXPECT_NE(static_cast<std::uint64_t>(*v0), static_cast<std::uint64_t>(*v65536))
            << name << ": u16-Truncation — key 0 und 65536 dürfen NICHT aliasen";
    }

    // (b3) update: bestehenden Key mit neuem Wert re-inserten → NICHT neu, Wert aktualisiert (insert_or_assign).
    {
        std::uint64_t const k   = 65536u;
        std::uint64_t const nv  = 0xABCDEF0123456789ull;
        bool const          org = c.insert(k, static_cast<V>(nv));
        EXPECT_FALSE(org) << name << ": update darf NICHT neu sein key=" << k;
        auto const ov = c.lookup(k);
        ASSERT_TRUE(ov.has_value());
        EXPECT_EQ(static_cast<std::uint64_t>(*ov), nv) << name << ": update-Wert key=" << k;
        oracle.insert_or_assign(k, nv);
        EXPECT_EQ(c.occupied_count(), oracle.size()) << name << ": occupied_count nach update key=" << k;
    }

    // (b4) miss: nie eingefügte Keys → kein Treffer.
    for (std::uint64_t k : {2ull, 100ull, 65534ull, 0x1234567800000000ull}) {
        if (oracle.find(k) == oracle.end())
            EXPECT_FALSE(c.lookup(k).has_value()) << name << ": miss erwartet key=" << k;
    }

    // (b5) erase jeden zweiten Key: bool-Rückgabe + occupied_count bit-identisch.
    for (std::size_t i = 0; i < keys.size(); i += 2) {
        std::uint64_t const k      = keys[i];
        bool const          org_er = c.erase(k);
        bool const          map_er = (oracle.erase(k) == 1u);
        EXPECT_EQ(org_er, map_er) << name << ": erase-Rückgabe key=" << k;
        EXPECT_EQ(c.occupied_count(), oracle.size()) << name << ": occupied_count nach erase key=" << k;
    }
    // erase eines bereits gelöschten Keys → false.
    EXPECT_FALSE(c.erase(keys[0])) << name << ": doppel-erase muss false sein key=" << keys[0];

    // (b6) verbleibende Keys weiter korrekt; gelöschte weg.
    for (std::size_t i = 0; i < keys.size(); ++i) {
        bool const should_hit = (oracle.find(keys[i]) != oracle.end());
        EXPECT_EQ(c.lookup(keys[i]).has_value(), should_hit) << name << ": Rest-Zustand key=" << keys[i];
    }
    expect_for_each_matches("nach erase");

    // (b7) clear leert vollständig.
    c.clear();
    oracle.clear();
    EXPECT_EQ(c.occupied_count(), 0u) << name << ": occupied_count nach clear";
    expect_for_each_matches("nach clear");
    EXPECT_FALSE(c.lookup(65536u).has_value()) << name << ": nach clear kein Treffer";
}

} // namespace

// Alle 9 registrierten Weg-B-Pool-Familien (organ_for_search_algo-gemappt) — genau die Menge, die 4b-b1 auf
// container_=ObservableComposedContainer<organ> umstellt. Namen 1:1 aus axis_03a_search_algo_registry.hpp.
TEST(Pool_Organ_Wide_Key_Conformance_188_4bb0, AllNinePoolFamiliesBehaveLikeStdMap) {
    verify_pool_organ_wide_key_conformance<lk::BinarySearchTreeSearchAlgo>("BST");
    verify_pool_organ_wide_key_conformance<lk::BTreeSearchAlgo>("BTree");
    verify_pool_organ_wide_key_conformance<lk::SkipListSearchAlgo>("SkipList");
    verify_pool_organ_wide_key_conformance<lk::HashSearchAlgo>("Hash");
    verify_pool_organ_wide_key_conformance<lk::OriginalArtSearchAlgo>("ART");
    verify_pool_organ_wide_key_conformance<lk::OriginalHotSearchAlgo>("HOT");
    verify_pool_organ_wide_key_conformance<lk::OriginalStartSearchAlgo>("START");
    verify_pool_organ_wide_key_conformance<lk::OriginalWormholeSearchAlgo>("Wormhole");
    verify_pool_organ_wide_key_conformance<lk::OriginalSurfSearchAlgo>("SuRF");
}
