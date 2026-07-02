// #234-F4 — HashProbeShape real: Load-Factor aus Shape + Chaining-Vollausbau.
// Deutscher Verhaltensbeleg gegen std::map<uint64,uint64>: OA_LF50/OA_LF90/CHAINING muessen insert/update/
// lookup/erase/clear und for_each_record exakt tragen; Level-0 HashOaLf70 bleibt am alten 7/10-Prädikat.

#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_chaining.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_oa_lf50.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_oa_lf70.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_oa_lf90.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <type_traits>
#include <vector>

namespace {

namespace lkc = ::comdare::cache_engine::lookup::composable;
namespace hps = ::comdare::cache_engine::nodes::axis_hash_probe_shape;

std::vector<std::uint64_t> wide_key_set_234_f4() {
    return {0u,
            1u,
            7u,
            42u,
            255u,
            256u,
            999u,
            65535u,
            65536u,
            65537u,
            131072u,
            (1ull << 20),
            (1ull << 32),
            (1ull << 33),
            (1ull << 40),
            (1ull << 48),
            0x00000000FFFFFFFFull,
            0xFFFFFFFFFFFF0000ull,
            0xDEADBEEFCAFEBABEull,
            0xFFFFFFFFFFFFFFFFull};
}

std::uint64_t value_for_234_f4(std::uint64_t k, std::uint64_t salt) {
    return (k ^ 0x9E3779B97F4A7C15ull) + salt * 0xBF58476D1CE4E5B9ull;
}

template <class Organ>
void expect_for_each_matches_234_f4(Organ const& organ, std::map<std::uint64_t, std::uint64_t> const& oracle,
                                    char const* name, char const* phase) {
    std::set<std::uint64_t>                  seen;
    std::map<std::uint64_t, std::uint64_t>   harvested;
    std::size_t const visits = organ.for_each_record([&](std::uint64_t k, std::uint64_t v) {
        bool const inserted = seen.insert(k).second;
        EXPECT_TRUE(inserted) << name << ": for_each_record Duplicate key=" << k << " phase=" << phase;
        harvested.emplace(k, v);
    });
    EXPECT_EQ(visits, oracle.size()) << name << ": for_each_record Rueckgabe phase=" << phase;
    EXPECT_EQ(visits, organ.occupied_count()) << name << ": for_each_record vs occupied_count phase=" << phase;
    EXPECT_EQ(seen.size(), oracle.size()) << name << ": for_each_record seen set phase=" << phase;
    EXPECT_EQ(harvested, oracle) << name << ": for_each_record Paare phase=" << phase;
}

template <class Shape>
void verify_shape_matches_std_map_234_f4(char const* name) {
    using Organ = lkc::HashSearchOrganShaped<Shape>;
    static_assert(std::is_same_v<typename Organ::key_type, std::uint64_t>);
    static_assert(std::is_same_v<typename Organ::value_type, std::uint64_t>);

    Organ                                  organ;
    std::map<std::uint64_t, std::uint64_t> oracle;
    std::vector<std::uint64_t>             touched = wide_key_set_234_f4();

    auto const upsert = [&](std::uint64_t k, std::uint64_t v, char const* phase) {
        organ.insert(k, v);
        oracle[k] = v;
        EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": occupied_count phase=" << phase << " key=" << k;
    };

    for (std::size_t i = 0; i < touched.size(); ++i) upsert(touched[i], value_for_234_f4(touched[i], i), "wide");

    std::mt19937_64 rng(0x234F40000000000ull + static_cast<std::uint64_t>(Shape::family_id::value));
    for (std::size_t i = 0; i < 6000; ++i) {
        std::uint64_t k = rng();
        if ((i % 11u) == 0u) k = (static_cast<std::uint64_t>(rng() & 0xFFFFu) << 16) | (i & 0xFFFFu);
        touched.push_back(k);
        upsert(k, value_for_234_f4(k, i + 1000u), "random>=5000");
    }

    for (std::size_t i = 0; i < touched.size(); i += 3) {
        std::uint64_t const k = touched[i];
        upsert(k, value_for_234_f4(k, i + 9000u), "update");
    }

    ASSERT_EQ(organ.occupied_count(), oracle.size()) << name << ": occupied_count nach update";
    for (auto const& kv : oracle) {
        auto const ov = organ.lookup(kv.first);
        ASSERT_TRUE(ov.has_value()) << name << ": lookup-Treffer erwartet key=" << kv.first;
        EXPECT_EQ(*ov, kv.second) << name << ": Wert key=" << kv.first;
    }
    for (std::uint64_t k : {2ull, 100ull, 65534ull, 0x1234567800000000ull, 0xCAFED00D00000000ull}) {
        if (!oracle.contains(k)) EXPECT_FALSE(organ.lookup(k).has_value()) << name << ": miss erwartet key=" << k;
    }
    expect_for_each_matches_234_f4(organ, oracle, name, "nach insert/update");

    for (std::size_t i = 0; i < touched.size(); i += 4) {
        std::uint64_t const k      = touched[i];
        bool const          org_er = organ.erase(k);
        bool const          map_er = (oracle.erase(k) == 1u);
        EXPECT_EQ(org_er, map_er) << name << ": erase-Rueckgabe key=" << k;
        EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": occupied_count nach erase key=" << k;
    }
    EXPECT_FALSE(organ.erase(touched.front())) << name << ": doppel-erase muss false sein";

    for (auto const& kv : oracle) {
        auto const ov = organ.lookup(kv.first);
        ASSERT_TRUE(ov.has_value()) << name << ": Rest-Lookup key=" << kv.first;
        EXPECT_EQ(*ov, kv.second) << name << ": Rest-Wert key=" << kv.first;
    }
    expect_for_each_matches_234_f4(organ, oracle, name, "nach erase");

    organ.clear();
    oracle.clear();
    EXPECT_EQ(organ.occupied_count(), 0u) << name << ": occupied_count nach clear";
    expect_for_each_matches_234_f4(organ, oracle, name, "nach clear");
    EXPECT_FALSE(organ.lookup(65536u).has_value()) << name << ": nach clear kein Treffer";

    for (std::uint64_t i = 0; i < 512; ++i) {
        std::uint64_t const k = (1ull << 48) + i * 65537ull;
        upsert(k, value_for_234_f4(k, i + 20000u), "refill");
    }
    expect_for_each_matches_234_f4(organ, oracle, name, "nach refill");
}

constexpr std::size_t trigger_occupancy_234_f4(std::size_t cap, int numerator, int denominator) {
    return (cap * static_cast<std::size_t>(numerator) + static_cast<std::size_t>(denominator) - 1u) /
           static_cast<std::size_t>(denominator);
}

} // namespace

TEST(Comdare234F4HashProbeShape, OaLf50ConformsToStdMapWideRandomAndClearRefill) {
    verify_shape_matches_std_map_234_f4<hps::HashOaLf50>("HashOaLf50");
}

TEST(Comdare234F4HashProbeShape, OaLf90ConformsToStdMapWideRandomAndClearRefill) {
    verify_shape_matches_std_map_234_f4<hps::HashOaLf90>("HashOaLf90");
}

TEST(Comdare234F4HashProbeShape, ChainingConformsToStdMapWideRandomAndClearRefill) {
    verify_shape_matches_std_map_234_f4<hps::HashChaining>("HashChaining");
}

TEST(Comdare234F4HashProbeShape, LoadFactorN64SeparatesLf50AndLf90) {
    constexpr std::size_t n64            = 64;
    constexpr std::size_t lf50_threshold = trigger_occupancy_234_f4(
        n64, hps::HashOaLf50::kLoadNumerator, hps::HashOaLf50::kLoadDenominator);
    constexpr std::size_t lf90_threshold = trigger_occupancy_234_f4(
        n64, hps::HashOaLf90::kLoadNumerator, hps::HashOaLf90::kLoadDenominator);
    static_assert(lf50_threshold == 32u);
    static_assert(lf90_threshold == 58u);

    lkc::HashSearchOrganShaped<hps::HashOaLf50> lf50;
    lkc::HashSearchOrganShaped<hps::HashOaLf90> lf90;
    for (std::uint64_t i = 0; i <= lf50_threshold; ++i) {
        lf50.insert(i, value_for_234_f4(i, 50u));
        lf90.insert(i, value_for_234_f4(i, 90u));
    }

    EXPECT_EQ(lf50.pool().bucket_count(), n64 * 2u) << "LF50 triggert bei N=64 ab occ=32 vor Insert 33";
    EXPECT_EQ(lf90.pool().bucket_count(), n64) << "LF90 bleibt bei N=64 bis occ=58";
    EXPECT_NE(lf50.pool().bucket_count(), lf90.pool().bucket_count());
}

TEST(Comdare234F4HashProbeShape, ChainingKeepsTombstonesZeroAndRecyclesNodes) {
    lkc::HashSearchOrganShaped<hps::HashChaining> organ;
    for (std::uint64_t i = 0; i < 256; ++i) organ.insert(i, value_for_234_f4(i, 1u));

    std::size_t const baseline_nodes = organ.pool().node_slot_count();
    ASSERT_EQ(organ.occupied_count(), 256u);
    ASSERT_EQ(baseline_nodes, 256u);
    EXPECT_EQ(organ.pool().tombstones(), 0u);

    for (std::uint64_t round = 0; round < 64; ++round) {
        for (std::uint64_t i = 0; i < 128; ++i) EXPECT_TRUE(organ.erase(i)) << "round=" << round << " key=" << i;
        EXPECT_EQ(organ.pool().tombstones(), 0u) << "round=" << round << " nach erase";
        EXPECT_EQ(organ.pool().node_slot_count(), baseline_nodes) << "Node-Array darf beim Erase nicht wachsen";

        for (std::uint64_t i = 0; i < 128; ++i) organ.insert(i, value_for_234_f4(i, round + 100u));
        EXPECT_EQ(organ.occupied_count(), 256u) << "round=" << round << " stabile Live-Menge";
        EXPECT_EQ(organ.pool().tombstones(), 0u) << "round=" << round << " nach reinsert";
        EXPECT_LE(organ.pool().node_slot_count(), baseline_nodes) << "Freiliste muss geloeschte Nodes recyceln";
    }
}

TEST(Comdare234F4HashProbeShape, Level0AnchorLf70KeepsDefaultAliasAndOldGrowthPoints) {
    static_assert(std::is_same_v<lkc::HashSearchOrgan, lkc::HashSearchOrganShaped<hps::HashOaLf70>>);
    static_assert(hps::HashOaLf70::kLoadNumerator == 7);
    static_assert(hps::HashOaLf70::kLoadDenominator == 10);

    auto const old_trigger = [](std::size_t occ, std::size_t tomb, std::size_t cap) {
        return (occ + tomb) * 10u >= cap * 7u;
    };
    EXPECT_FALSE(old_trigger(11u, 0u, 16u));
    EXPECT_TRUE(old_trigger(12u, 0u, 16u));
    EXPECT_FALSE(old_trigger(22u, 0u, 32u));
    EXPECT_TRUE(old_trigger(23u, 0u, 32u));
    EXPECT_FALSE(old_trigger(44u, 0u, 64u));
    EXPECT_TRUE(old_trigger(45u, 0u, 64u));

    lkc::HashSearchOrganShaped<hps::HashOaLf70> organ;
    for (std::uint64_t i = 0; i < 12; ++i) organ.insert(i, value_for_234_f4(i, 70u));
    EXPECT_EQ(organ.pool().bucket_count(), 16u);
    organ.insert(12u, value_for_234_f4(12u, 70u));
    EXPECT_EQ(organ.pool().bucket_count(), 32u);

    for (std::uint64_t i = 13; i < 23; ++i) organ.insert(i, value_for_234_f4(i, 70u));
    EXPECT_EQ(organ.pool().bucket_count(), 32u);
    organ.insert(23u, value_for_234_f4(23u, 70u));
    EXPECT_EQ(organ.pool().bucket_count(), 64u);

    for (std::uint64_t i = 24; i < 45; ++i) organ.insert(i, value_for_234_f4(i, 70u));
    EXPECT_EQ(organ.pool().bucket_count(), 64u);
    organ.insert(45u, value_for_234_f4(45u, 70u));
    EXPECT_EQ(organ.pool().bucket_count(), 128u);
}
