// AP-7a/#241 -- SwissTable S22 Wrapper-Konformitaet und Gruppen-Probe-Shape.

#include <axes/lookup/axis_03a_search_algo_swisstable.hpp>

#include "support/std_map_equivalence_harness.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <vector>

namespace {

using ::comdare::cache_engine::lookup::SwissTableSearchAlgo;

std::uint64_t ap7_value_for(std::uint16_t k, std::uint64_t salt) {
    return (static_cast<std::uint64_t>(k) << 32u) ^ (salt * 0x9E3779B97F4A7C15ull);
}

void expect_matches_oracle(SwissTableSearchAlgo const& table, std::map<std::uint16_t, std::uint64_t> const& oracle,
                           std::vector<std::uint16_t> const& touched, char const* phase) {
    EXPECT_EQ(table.occupied_count(), oracle.size()) << phase;
    for (auto const& kv : oracle) {
        auto const got = table.lookup(kv.first);
        ASSERT_TRUE(got.has_value()) << phase << " key=" << static_cast<std::uint32_t>(kv.first);
        EXPECT_EQ(*got, kv.second) << phase << " key=" << static_cast<std::uint32_t>(kv.first);
    }
    for (std::uint16_t k : touched) {
        if (!oracle.contains(k)) EXPECT_FALSE(table.lookup(k).has_value()) << phase << " erased key=" << k;
    }
}

std::vector<std::uint16_t> h2_collision_keys() {
    std::vector<std::uint16_t> keys;
    keys.reserve(320u);
    for (std::uint16_t h2_class = 0; h2_class < 4u; ++h2_class) {
        for (std::uint16_t n = 0; n < 80u; ++n) keys.push_back(static_cast<std::uint16_t>(h2_class + 128u * n));
    }
    return keys;
}

} // namespace

static_assert(SwissTableSearchAlgo::family_id::value == 22);
static_assert(!SwissTableSearchAlgo::supports_simd());
static_assert(!SwissTableSearchAlgo::supports_range_scan());
static_assert(!SwissTableSearchAlgo::is_dense());
static_assert(SwissTableSearchAlgo::flag_suffix() == "SWISSTABLE");

TEST(ComdareAP7SwissTable, ConformsToStdMapHarness) {
    ::comdare::cache_engine::test_support::verify_matches_std_map<SwissTableSearchAlgo>(100000u, 2000u);
}

TEST(ComdareAP7SwissTable, ScalarGroupProbeH2TombstoneClearAndRefill) {
    SwissTableSearchAlgo                   table;
    std::map<std::uint16_t, std::uint64_t> oracle;
    std::vector<std::uint16_t> const       touched = h2_collision_keys();

    for (std::size_t i = 0; i < touched.size(); ++i) {
        std::uint16_t const k = touched[i];
        table.insert(k, ap7_value_for(k, i));
        oracle[k] = ap7_value_for(k, i);
        EXPECT_EQ(table.occupied_count(), oracle.size()) << "insert key=" << k;
    }
    expect_matches_oracle(table, oracle, touched, "after initial H2-collision inserts");

    for (std::size_t i = 0; i < touched.size(); i += 3u) {
        std::uint16_t const k = touched[i];
        table.insert(k, ap7_value_for(k, i + 9000u));
        oracle[k] = ap7_value_for(k, i + 9000u);
    }
    expect_matches_oracle(table, oracle, touched, "after updates");

    for (std::size_t i = 0; i < touched.size(); i += 5u) {
        std::uint16_t const k   = touched[i];
        bool const          got = table.erase(k);
        bool const          ref = (oracle.erase(k) == 1u);
        EXPECT_EQ(got, ref) << "erase key=" << k;
        EXPECT_EQ(table.occupied_count(), oracle.size()) << "after erase key=" << k;
    }
    EXPECT_FALSE(table.erase(touched.front())) << "double erase must miss";
    expect_matches_oracle(table, oracle, touched, "after tombstone erases");

    for (std::size_t i = 1; i < touched.size(); i += 7u) {
        std::uint16_t const k = touched[i];
        table.insert(k, ap7_value_for(k, i + 17000u));
        oracle[k] = ap7_value_for(k, i + 17000u);
    }
    expect_matches_oracle(table, oracle, touched, "after tombstone reuse inserts");

    table.clear();
    oracle.clear();
    EXPECT_EQ(table.occupied_count(), 0u);
    for (std::uint16_t k : touched) EXPECT_FALSE(table.lookup(k).has_value()) << "after clear key=" << k;

    for (std::size_t i = 0; i < 64u; ++i) {
        std::uint16_t const k = static_cast<std::uint16_t>(60000u + i);
        table.insert(k, ap7_value_for(k, i + 23000u));
        oracle[k] = ap7_value_for(k, i + 23000u);
    }
    std::vector<std::uint16_t> refill_keys;
    for (auto const& kv : oracle) refill_keys.push_back(kv.first);
    expect_matches_oracle(table, oracle, refill_keys, "after clear refill");
}
