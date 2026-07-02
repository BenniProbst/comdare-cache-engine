// #188-4a (2026-07-02) -- Eytzinger organ_for-Familie: Store-traversierbar via EytzingerOrgan.
//
// GTest im Nachbar-Stil der #234-F-Tests: direkter Organ-Drive gegen std::map, Wrapper-Aequivalenz,
// Option-b-Lazy-Semantik und for_each_record-Ernte. Additiv/gate-frei: NICHT in CMake registriert.

#include <axes/lookup/axis_03a_search_algo_eytzinger.hpp>
#include <axes/lookup/composable/organ_for_search_algo.hpp>
#include <axes/lookup/composable/store_traversable_search_algo.hpp>
#include <axes/lookup/composable/traversal_for_search_algo.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <type_traits>
#include <vector>

namespace {

namespace lk  = ::comdare::cache_engine::lookup;
namespace lkc = ::comdare::cache_engine::lookup::composable;

using U64 = std::uint64_t;
using U16 = std::uint16_t;

static_assert(std::is_same_v<lkc::organ_for_search_algo_t<lk::EytzingerSearchAlgo>, lkc::EytzingerOrgan>);
static_assert(std::is_same_v<lkc::traversal_for_search_algo_t<lk::EytzingerSearchAlgo>, void>);
static_assert(lkc::EytzingerLayoutPool<lkc::EytzingerLayoutStore>);
static_assert(!lkc::StoreTraversableSearchAlgo<lk::EytzingerSearchAlgo>);

constexpr U64 kSeedT1       = 0x1884A001C0FFEEull;
constexpr U64 kSeedT2       = 0x1884A002C0FFEEull;
constexpr int kMixedOps     = 700;
constexpr int kWrapperOps   = 420;
constexpr U64 kKeyMask      = 0x1FFFull;
constexpr U64 kValueSalt    = 0x9E3779B97F4A7C15ull;
constexpr U64 kUpdateSalt   = 0xD1B54A32D192ED03ull;
constexpr U64 kMissingValue = 0xFFFFFFFFFFFFFFFFull;

[[nodiscard]] U64 value_for(U64 key, U64 salt) noexcept { return (key ^ kValueSalt) + (salt * kUpdateSalt); }

template <class Organ>
void expect_lookup_matches_map(Organ const& organ, std::map<U64, U64> const& oracle, U64 key, char const* phase) {
    std::optional<U64> const actual = organ.lookup(key);
    auto const               it     = oracle.find(key);
    if (it == oracle.end()) {
        EXPECT_FALSE(actual.has_value()) << phase << ": miss erwartet key=" << key;
        return;
    }
    ASSERT_TRUE(actual.has_value()) << phase << ": hit erwartet key=" << key;
    EXPECT_EQ(*actual, it->second) << phase << ": Wertdrift key=" << key;
}

void expect_wrapper_matches_organ(lk::EytzingerSearchAlgo const& wrapper, lkc::EytzingerOrgan const& organ, U16 key,
                                  char const* phase) {
    auto const w = wrapper.lookup(key);
    auto const o = organ.lookup(static_cast<U64>(key));
    EXPECT_EQ(w.has_value(), o.has_value()) << phase << ": hit/miss drift key=" << key;
    if (w && o) { EXPECT_EQ(*w, *o) << phase << ": Wertdrift key=" << key; }
}
void expect_optional_value(std::optional<U64> const& actual, U64 expected, char const* phase) {
    ASSERT_TRUE(actual.has_value()) << phase << ": Treffer erwartet";
    EXPECT_EQ(*actual, expected) << phase << ": Wertdrift";
}

} // namespace

TEST(Eytzinger_Organ_188_4a, T1_ConformanceAgainstStdMap) {
    lkc::EytzingerOrgan organ;
    std::map<U64, U64>  oracle;
    std::mt19937_64     rng{kSeedT1};
    std::vector<U64>    touched;

    for (int op_index = 0; op_index < kMixedOps; ++op_index) {
        U64 const raw_key = rng();
        U64 const key     = ((op_index % 11) == 0) ? raw_key : (raw_key & kKeyMask);
        touched.push_back(key);
        int const op = static_cast<int>(rng() % 5u);
        if (op <= 1) {
            U64 const value = value_for(key, static_cast<U64>(op_index + 1));
            organ.insert(key, value);
            oracle.insert_or_assign(key, value);
        } else if (op == 2) {
            bool const org_erased = organ.erase(key);
            bool const map_erased = (oracle.erase(key) != 0u);
            EXPECT_EQ(org_erased, map_erased) << "erase Rueckgabe key=" << key;
        } else {
            expect_lookup_matches_map(organ, oracle, key, "lookup-op");
        }

        EXPECT_EQ(organ.occupied_count(), oracle.size()) << "occupied_count op=" << op_index;
        if ((op_index % 37) == 0) {
            expect_lookup_matches_map(organ, oracle, key, "checkpoint-current");
            for (std::size_t i = 0; i < touched.size(); i += 53u)
                expect_lookup_matches_map(organ, oracle, touched[i], "checkpoint-touched");
        }
    }

    for (auto const& kv : oracle) expect_lookup_matches_map(organ, oracle, kv.first, "final-sweep");
    EXPECT_FALSE(organ.lookup(kMissingValue).has_value()) << "final miss fuer reservierten Key";
}

TEST(Eytzinger_Organ_188_4a, T2_WrapperEquivalenceInU16Keyspace) {
    lk::EytzingerSearchAlgo wrapper;
    lkc::EytzingerOrgan     organ;
    std::mt19937_64         rng{kSeedT2};
    std::vector<U16>        touched;

    for (int op_index = 0; op_index < kWrapperOps; ++op_index) {
        U16 const key = static_cast<U16>(rng() & 0x0FFFu);
        touched.push_back(key);
        int const op = static_cast<int>(rng() % 4u);
        if (op <= 1) {
            U64 const value = value_for(static_cast<U64>(key), static_cast<U64>(op_index + 7));
            wrapper.insert(key, value);
            organ.insert(static_cast<U64>(key), value);
        } else if (op == 2) {
            EXPECT_EQ(wrapper.erase(key), organ.erase(static_cast<U64>(key))) << "erase drift key=" << key;
        } else {
            expect_wrapper_matches_organ(wrapper, organ, key, "lookup-op");
        }

        EXPECT_EQ(wrapper.occupied_count(), organ.occupied_count()) << "size drift op=" << op_index;
        if ((op_index % 41) == 0) {
            expect_wrapper_matches_organ(wrapper, organ, key, "checkpoint-current");
            for (std::size_t i = 0; i < touched.size(); i += 47u)
                expect_wrapper_matches_organ(wrapper, organ, touched[i], "checkpoint-touched");
        }
    }
}

TEST(Eytzinger_Organ_188_4a, T3_OptionBLazyDirtySemantics) {
    lkc::EytzingerLayoutStore store;

    lkc::EytzingerTraversalOrgan::insert_into(store, 10u, 100u);
    EXPECT_TRUE(store.dirty()) << "insert invalidiert BFS-Puffer";
    expect_optional_value(lkc::EytzingerTraversalOrgan::lookup_in(store, 10u), 100u, "insert lookup");
    EXPECT_FALSE(store.dirty()) << "erster lookup baut lazy neu";
    expect_optional_value(lkc::EytzingerTraversalOrgan::lookup_in(store, 10u), 100u, "insert lookup");
    EXPECT_FALSE(store.dirty()) << "weiterer lookup ohne Mutation bleibt clean";

    EXPECT_TRUE(lkc::EytzingerTraversalOrgan::erase_from(store, 10u));
    EXPECT_TRUE(store.dirty()) << "erase invalidiert";
    lkc::EytzingerTraversalOrgan::insert_into(store, 11u, 111u);
    expect_optional_value(lkc::EytzingerTraversalOrgan::lookup_in(store, 11u), 111u, "reinsert lookup");
    EXPECT_FALSE(store.dirty());
    lkc::EytzingerTraversalOrgan::insert_into(store, 11u, 222u);
    EXPECT_TRUE(store.dirty()) << "update via set_value_at invalidiert ebenfalls";
    expect_optional_value(lkc::EytzingerTraversalOrgan::lookup_in(store, 11u), 222u, "update lookup");
    EXPECT_FALSE(store.dirty());
}

TEST(Eytzinger_Organ_188_4a, T4_ForEachRecordHarvestsSortedPrimaryWithoutRebuild) {
    lkc::EytzingerOrgan    organ;
    std::map<U64, U64>     oracle;
    std::vector<U64> const keys{40u, 10u, 30u, 20u, 50u, 60u};

    for (U64 key : keys) {
        organ.insert(key, value_for(key, 4u));
        oracle.insert_or_assign(key, value_for(key, 4u));
    }
    EXPECT_TRUE(organ.pool().dirty());
    EXPECT_TRUE(organ.erase(30u));
    oracle.erase(30u);
    EXPECT_TRUE(organ.pool().dirty());

    std::map<U64, U64> harvested;
    std::vector<U64>   visited_keys;
    std::size_t const  visited = organ.for_each_record([&](U64 key, U64 value) {
        visited_keys.push_back(key);
        harvested.emplace(key, value);
    });

    EXPECT_EQ(visited, organ.occupied_count());
    EXPECT_EQ(visited, oracle.size());
    EXPECT_EQ(harvested, oracle);
    EXPECT_TRUE(std::is_sorted(visited_keys.begin(), visited_keys.end()));
    EXPECT_TRUE(organ.pool().dirty()) << "for_each_record darf keinen lazy Rebuild triggern";
}

TEST(Eytzinger_Organ_188_4a, T5_BranchFreeBoundaryCases) {
    lkc::EytzingerOrgan organ;
    EXPECT_FALSE(organ.lookup(7u).has_value()) << "leeres Organ: miss";

    organ.insert(10u, 100u);
    expect_optional_value(organ.lookup(10u), 100u, "single hit");
    EXPECT_FALSE(organ.lookup(9u).has_value());
    EXPECT_FALSE(organ.lookup(11u).has_value());

    organ.insert(20u, 200u);
    organ.insert(30u, 300u);
    expect_optional_value(organ.lookup(10u), 100u, "kleinster Key");
    expect_optional_value(organ.lookup(30u), 300u, "groesster Key");
    EXPECT_FALSE(organ.lookup(19u).has_value());
    EXPECT_FALSE(organ.lookup(21u).has_value());
    organ.insert(20u, 222u);
    expect_optional_value(organ.lookup(20u), 222u, "Duplikat-Update liefert neuen Wert");
}

TEST(Eytzinger_Organ_188_4a, T6_CopyMementoKeepsDirtySnapshotIndependent) {
    lkc::EytzingerOrgan original;
    original.insert(1u, 10u);
    original.insert(2u, 20u);
    ASSERT_TRUE(original.pool().dirty());

    lkc::EytzingerOrgan copy = original;
    ASSERT_TRUE(copy.pool().dirty());

    original.insert(1u, 99u);
    original.insert(3u, 30u);

    expect_optional_value(copy.lookup(1u), 10u, "copy key 1");
    expect_optional_value(copy.lookup(2u), 20u, "copy key 2");
    EXPECT_FALSE(copy.lookup(3u).has_value());
    expect_optional_value(original.lookup(1u), 99u, "original key 1");
    expect_optional_value(original.lookup(3u), 30u, "original key 3");
}

TEST(Eytzinger_Organ_188_4a, T7_ObservableHullTracksInsertFlagsAndStatistics) {
    lkc::ObservableEytzingerOrgan hull;

    EXPECT_TRUE(hull.insert(5u, 50u));
    EXPECT_FALSE(hull.insert(5u, 51u));
    EXPECT_EQ(hull.occupied_count(), 1u);
    expect_optional_value(hull.lookup(5u), 51u, "observable hit");
    EXPECT_FALSE(hull.lookup(6u).has_value());
    EXPECT_TRUE(hull.erase(5u));
    EXPECT_FALSE(hull.erase(5u));

#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const stats = hull.statistics();
    EXPECT_EQ(stats.total_insert_count, 2u);
    EXPECT_EQ(stats.peak_occupancy, 1u);
    EXPECT_EQ(stats.total_lookup_count, 2u);
    EXPECT_EQ(stats.total_hit_count, 1u);
    EXPECT_EQ(stats.total_miss_count, 1u);
    EXPECT_EQ(stats.total_erase_count, 1u);
#endif
}

TEST(Eytzinger_Organ_188_4a, T8_StaticTraitAssertionsAreCompiled) {
    SUCCEED() << "compile-time static_asserts stehen am Dateikopf";
}
