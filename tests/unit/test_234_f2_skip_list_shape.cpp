// #234-F2 (2026-07-02) - SkipListShape-Varianten real: bit-treue P-Verallgemeinerung in draw_level.
//
// Deutscher Konformitaets-Banner: Dieser Test belegt den Level-0-Vertrag fuer P=1/2 draw-fuer-draw gegen die
// alte Muenzwurf-Formel, verankert P=1/4 statistisch und treibt die realen SkipListOrganShaped-Varianten gegen
// std::map ueber einen weiten uint64-Schluesselraum inklusive for_each_record-exactly-once.

#include <axes/lookup/composable/skip_list_node_pool_store.hpp>
#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_max16_p25.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_max16_p50.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_max32_p50.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_max8_p50.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <type_traits>
#include <vector>

namespace {

namespace lkc     = ::comdare::cache_engine::lookup::composable;
namespace slshape = ::comdare::cache_engine::nodes::axis_skip_list_shape;

static_assert(std::is_same_v<lkc::SkipListOrgan, lkc::SkipListOrganShaped<slshape::SkipListMax16P50>>);

std::vector<std::uint64_t> wide_key_set_234_f2() {
    std::vector<std::uint64_t> keys{
        0u, 7u, 65535u, 65536u, (1ull << 32), (1ull << 40), std::numeric_limits<std::uint64_t>::max()};
    std::mt19937_64 rng(0x234F2u);
    for (int i = 0; i < 6000; ++i) keys.push_back(rng());
    return keys;
}

std::uint64_t value_for_234_f2(std::uint64_t key, std::size_t salt) {
    return key ^ 0x9E3779B97F4A7C15ull ^ (static_cast<std::uint64_t>(salt) * 0xD1B54A32D192ED03ull);
}

template <class Organ>
void expect_for_each_matches_234_f2(Organ const& organ, std::map<std::uint64_t, std::uint64_t> const& oracle,
                                    char const* name, char const* phase) {
    std::map<std::uint64_t, std::uint64_t> harvested;
    std::set<std::uint64_t>                seen;
    std::size_t const                      visits = organ.for_each_record([&](std::uint64_t key, std::uint64_t value) {
        bool const seen_new = seen.insert(key).second;
        EXPECT_TRUE(seen_new) << name << ": for_each_record Duplicate key=" << key << " phase=" << phase;
        bool const harvested_new = harvested.emplace(key, value).second;
        EXPECT_TRUE(harvested_new) << name << ": for_each_record Duplicate harvest key=" << key << " phase=" << phase;
    });
    EXPECT_EQ(visits, oracle.size()) << name << ": for_each_record Rueckgabe phase=" << phase;
    EXPECT_EQ(visits, organ.occupied_count()) << name << ": for_each_record vs occupied_count phase=" << phase;
    EXPECT_EQ(harvested, oracle) << name << ": for_each_record Paare phase=" << phase;
}

template <class Organ>
void expect_lookups_match_234_f2(Organ const& organ, std::map<std::uint64_t, std::uint64_t> const& oracle,
                                 std::vector<std::uint64_t> const& keys, char const* name, char const* phase) {
    for (std::uint64_t key : keys) {
        auto const got = organ.lookup(key);
        auto const it  = oracle.find(key);
        if (it == oracle.end()) {
            EXPECT_FALSE(got.has_value()) << name << ": lookup-Miss erwartet key=" << key << " phase=" << phase;
        } else {
            ASSERT_TRUE(got.has_value()) << name << ": lookup-Treffer erwartet key=" << key << " phase=" << phase;
            EXPECT_EQ(*got, it->second) << name << ": lookup-Wert key=" << key << " phase=" << phase;
        }
    }
}

template <class Shape>
void verify_skip_list_shape_conformance_234_f2(char const* name) {
    using Organ = lkc::SkipListOrganShaped<Shape>;
    static_assert(std::is_same_v<typename Organ::key_type, std::uint64_t>);
    static_assert(std::is_same_v<typename Organ::value_type, std::uint64_t>);

    Organ                                  organ;
    std::map<std::uint64_t, std::uint64_t> oracle;
    std::vector<std::uint64_t> const       keys = wide_key_set_234_f2();
    std::vector<std::uint64_t> const       fixed_keys(keys.begin(), keys.begin() + 7);

    for (std::size_t i = 0; i < keys.size(); ++i) {
        std::uint64_t const key   = keys[i];
        std::uint64_t const value = value_for_234_f2(key, i);
        organ.insert(key, value);
        oracle.insert_or_assign(key, value);
        EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": occupied_count nach insert i=" << i;

        auto const got = organ.lookup(key);
        ASSERT_TRUE(got.has_value()) << name << ": lookup nach insert key=" << key;
        EXPECT_EQ(*got, value) << name << ": Wert nach insert key=" << key;

        if ((i % 5u) == 0u) {
            std::uint64_t const updated = value_for_234_f2(key, i + keys.size());
            organ.insert(key, updated);
            oracle.insert_or_assign(key, updated);
            auto const after_update = organ.lookup(key);
            ASSERT_TRUE(after_update.has_value()) << name << ": lookup nach update key=" << key;
            EXPECT_EQ(*after_update, updated) << name << ": update-Wert key=" << key;
            EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": occupied_count nach update i=" << i;
        }

        if (i > 0u && (i % 11u) == 0u) {
            std::uint64_t const victim = keys[i / 2u];
            bool const          org_er = organ.erase(victim);
            bool const          map_er = (oracle.erase(victim) == 1u);
            EXPECT_EQ(org_er, map_er) << name << ": erase-Rueckgabe victim=" << victim << " i=" << i;
            EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": occupied_count nach erase i=" << i;
        }

        if ((i % 37u) == 0u) {
            std::uint64_t const probe = key ^ 0xA5A5A5A5A5A5A5A5ull;
            auto const          org_v = organ.lookup(probe);
            auto const          map_v = oracle.find(probe);
            if (map_v == oracle.end()) {
                EXPECT_FALSE(org_v.has_value()) << name << ": lookup-Miss probe=" << probe << " i=" << i;
            } else {
                ASSERT_TRUE(org_v.has_value()) << name << ": lookup-Treffer probe=" << probe << " i=" << i;
                EXPECT_EQ(*org_v, map_v->second) << name << ": lookup-Probe-Wert i=" << i;
            }
        }
    }

    expect_lookups_match_234_f2(organ, oracle, keys, name, "phase-1");
    expect_for_each_matches_234_f2(organ, oracle, name, "phase-1");

    organ.clear();
    oracle.clear();
    EXPECT_EQ(organ.occupied_count(), 0u) << name << ": occupied_count nach clear";
    expect_lookups_match_234_f2(organ, oracle, fixed_keys, name, "nach-clear");
    expect_for_each_matches_234_f2(organ, oracle, name, "nach-clear");

    for (std::size_t i = keys.size(); i-- > 0u;) {
        if ((i % 3u) == 0u) continue;
        std::uint64_t const key   = keys[i];
        std::uint64_t const value = value_for_234_f2(key, i + 2u * keys.size());
        organ.insert(key, value);
        oracle.insert_or_assign(key, value);

        if ((i % 13u) == 0u) {
            std::uint64_t const updated = value_for_234_f2(key, i + 3u * keys.size());
            organ.insert(key, updated);
            oracle.insert_or_assign(key, updated);
        }
        if ((i % 17u) == 0u) {
            bool const org_er = organ.erase(key);
            bool const map_er = (oracle.erase(key) == 1u);
            EXPECT_EQ(org_er, map_er) << name << ": phase-2 erase-Rueckgabe key=" << key << " i=" << i;
        }
        EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": phase-2 occupied_count i=" << i;
    }

    expect_lookups_match_234_f2(organ, oracle, keys, name, "phase-2");
    expect_for_each_matches_234_f2(organ, oracle, name, "phase-2");
}

template <class Shape>
void expect_level_bound_234_f2(int expected_max, char const* name) {
    lkc::SkipListNodePoolStore<Shape> store;
    for (int i = 0; i < 50000; ++i) {
        int const lvl = store.draw_level();
        EXPECT_GE(lvl, 1) << name << ": Level-Untergrenze i=" << i;
        EXPECT_LE(lvl, expected_max) << name << ": Level-Obergrenze i=" << i;
    }
}

} // namespace

TEST(Comdare234F2SkipListShape, DefaultP50LevelSequenceIsBitTreueToOldFormula) {
    lkc::SkipListNodePoolStore<> store;
    std::mt19937_64              alt(0xC0FFEEu);

    for (int i = 0; i < 12000; ++i) {
        int lvl = 1;
        while ((alt() & 1u) != 0u && lvl < 16) ++lvl;
        EXPECT_EQ(store.draw_level(), lvl) << "#234-F2 bit-treue Level-Sequenz i=" << i;
    }
}

TEST(Comdare234F2SkipListShape, P25StatisticsAnchorMatchesQuarterAndSixteenth) {
    lkc::SkipListNodePoolStore<slshape::SkipListMax16P25> store;
    constexpr int                                         kDraws = 100000;
    int                                                   ge2    = 0;
    int                                                   ge3    = 0;

    for (int i = 0; i < kDraws; ++i) {
        int const lvl = store.draw_level();
        EXPECT_GE(lvl, 1) << "#234-F2 P25 Level-Untergrenze i=" << i;
        EXPECT_LE(lvl, 16) << "#234-F2 P25 Level-Obergrenze i=" << i;
        if (lvl >= 2) ++ge2;
        if (lvl >= 3) ++ge3;
    }

    EXPECT_NEAR(static_cast<double>(ge2) / static_cast<double>(kDraws), 0.25, 0.02);
    EXPECT_NEAR(static_cast<double>(ge3) / static_cast<double>(kDraws), 0.0625, 0.01);
}

TEST(Comdare234F2SkipListShape, Max8AndMax32LevelBoundsHold) {
    expect_level_bound_234_f2<slshape::SkipListMax8P50>(8, "Max8P50");
    expect_level_bound_234_f2<slshape::SkipListMax32P50>(32, "Max32P50");
}

TEST(Comdare234F2SkipListShape, ShapedOrgansConformToStdMap) {
    verify_skip_list_shape_conformance_234_f2<slshape::SkipListMax8P50>("Max8P50");
    verify_skip_list_shape_conformance_234_f2<slshape::SkipListMax32P50>("Max32P50");
    verify_skip_list_shape_conformance_234_f2<slshape::SkipListMax16P25>("Max16P25");
}

TEST(Comdare234F2SkipListShape, Level0AliasIsDefaultShape) {
    SUCCEED() << "#234-F2 static_assert: SkipListOrgan == SkipListOrganShaped<SkipListMax16P50>";
}