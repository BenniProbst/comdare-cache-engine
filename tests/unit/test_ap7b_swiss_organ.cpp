// AP-7b/#262 -- SwissTableOrgan Weg-B native organ conformance.

#include <axes/lookup/composable/tier_to_organ_mapping.hpp>

#include "builder/measurement_snapshot.hpp"
#include <anatomy/observable_tier.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace b       = ::comdare::cache_engine::builder;
namespace anatomy = ::comdare::cache_engine::anatomy;
namespace lkc     = ::comdare::cache_engine::lookup::composable;

namespace {

using SwissTableOrgan = lkc::SwissTableOrgan;

[[nodiscard]] std::uint64_t ap7b_value_for(std::uint64_t k, std::uint64_t salt) {
    return (k << 1u) ^ (salt * 0x9E3779B97F4A7C15ull);
}

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

std::vector<std::uint64_t> h2_collision_keys() {
    std::vector<std::uint64_t> keys;
    keys.reserve(320u);
    for (std::uint64_t h2_class = 0; h2_class < 4u; ++h2_class) {
        for (std::uint64_t n = 0; n < 80u; ++n) keys.push_back((1ull << 32u) + h2_class + 128ull * n);
    }
    return keys;
}

void expect_for_each_exactly_once(SwissTableOrgan const& organ, std::map<std::uint64_t, std::uint64_t> const& oracle,
                                  char const* phase) {
    std::map<std::uint64_t, std::uint64_t> harvested;
    std::size_t const                      visits = organ.for_each_record([&](std::uint64_t k, std::uint64_t v) {
        bool const inserted = harvested.emplace(k, v).second;
        EXPECT_TRUE(inserted) << phase << " duplicate key=" << k;
    });
    EXPECT_EQ(visits, oracle.size()) << phase;
    EXPECT_EQ(visits, organ.occupied_count()) << phase;
    EXPECT_EQ(harvested, oracle) << phase;
}

void expect_matches_oracle(SwissTableOrgan const& organ, std::map<std::uint64_t, std::uint64_t> const& oracle,
                           std::vector<std::uint64_t> const& touched, char const* phase) {
    EXPECT_EQ(organ.occupied_count(), oracle.size()) << phase;
    for (auto const& kv : oracle) {
        auto const got = organ.lookup(kv.first);
        ASSERT_TRUE(got.has_value()) << phase << " key=" << kv.first;
        EXPECT_EQ(*got, kv.second) << phase << " key=" << kv.first;
    }
    for (std::uint64_t k : touched) {
        if (!oracle.contains(k)) EXPECT_FALSE(organ.lookup(k).has_value()) << phase << " erased key=" << k;
    }
    expect_for_each_exactly_once(organ, oracle, phase);
}

} // namespace

static_assert(lkc::SwissGroupPool<lkc::SwissGroupPoolStore<>>);
static_assert(lkc::SwissGroupProbeTraversal<lkc::SwissGroupProbeTraversalOrgan, lkc::SwissGroupPoolStore<>>);
static_assert(std::same_as<typename SwissTableOrgan::key_type, std::uint64_t>);
static_assert(std::same_as<typename SwissTableOrgan::value_type, std::uint64_t>);

TEST(ComdareAP7bSwissTableOrgan, ScalarGroupProbeH2TombstoneRefillAndForEachRecord) {
    SwissTableOrgan                        organ;
    std::map<std::uint64_t, std::uint64_t> oracle;
    std::vector<std::uint64_t> const       touched = h2_collision_keys();
    std::vector<std::uint64_t>             erased;

    for (std::size_t i = 0; i < touched.size(); ++i) {
        std::uint64_t const k = touched[i];
        organ.insert(k, ap7b_value_for(k, i));
        oracle[k] = ap7b_value_for(k, i);
        EXPECT_EQ(organ.occupied_count(), oracle.size()) << "insert key=" << k;
    }
    expect_matches_oracle(organ, oracle, touched, "after initial H2-collision inserts");

    for (std::size_t i = 0; i < touched.size(); i += 3u) {
        std::uint64_t const k = touched[i];
        organ.insert(k, ap7b_value_for(k, i + 9000u));
        oracle[k] = ap7b_value_for(k, i + 9000u);
    }
    expect_matches_oracle(organ, oracle, touched, "after updates");

    for (std::size_t i = 0; i < touched.size(); i += 5u) {
        std::uint64_t const k   = touched[i];
        bool const          got = organ.erase(k);
        bool const          ref = (oracle.erase(k) == 1u);
        if (got) erased.push_back(k);
        EXPECT_EQ(got, ref) << "erase key=" << k;
        EXPECT_EQ(organ.occupied_count(), oracle.size()) << "after erase key=" << k;
    }
    ASSERT_FALSE(erased.empty());
    EXPECT_FALSE(organ.erase(erased.front())) << "double erase must miss";
    expect_matches_oracle(organ, oracle, touched, "after tombstone erases");

    for (std::size_t i = 0; i < erased.size(); i += 2u) {
        std::uint64_t const k = erased[i];
        organ.insert(k, ap7b_value_for(k, i + 17000u));
        oracle[k] = ap7b_value_for(k, i + 17000u);
    }
    expect_matches_oracle(organ, oracle, touched, "after tombstone refill inserts");

    organ.clear();
    oracle.clear();
    EXPECT_EQ(organ.occupied_count(), 0u);
    expect_for_each_exactly_once(organ, oracle, "after clear");
    for (std::uint64_t k : touched) EXPECT_FALSE(organ.lookup(k).has_value()) << "after clear key=" << k;

    std::vector<std::uint64_t> refill_keys;
    for (std::size_t i = 0; i < 64u; ++i) {
        std::uint64_t const k = (1ull << 48u) + i * 257u;
        organ.insert(k, ap7b_value_for(k, i + 23000u));
        oracle[k] = ap7b_value_for(k, i + 23000u);
        refill_keys.push_back(k);
    }
    expect_matches_oracle(organ, oracle, refill_keys, "after clear refill");
}

TEST(ComdareAP7bSwissTableOrgan, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<anatomy::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 5);
    EXPECT_EQ(sizeof(anatomy::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(anatomy::kTierObserverSnapshotVersionUnified, 5u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string>                     ids{"neutrality_guard"};
    std::vector<std::string>                     workloads{"ap9"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}
