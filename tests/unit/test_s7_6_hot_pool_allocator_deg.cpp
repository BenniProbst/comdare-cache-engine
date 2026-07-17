// S7-6 (#261/#234) -- HOT-Pool allocator parameter + T6 DEG hook.

#include "builder/measurement_snapshot.hpp"

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/lookup/axis_03a_search_algo_original_hot.hpp>
#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <builder/codegen/all_axes_umbrella.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <compositions/art_reference.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace an   = ::comdare::cache_engine::anatomy;
namespace b    = ::comdare::cache_engine::builder;
namespace comp = ::comdare::cache_engine::compositions;
namespace lk   = ::comdare::cache_engine::lookup;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

using U64          = std::uint64_t;
using DefaultStore = lkc::HotPatriciaNodePoolStore<>;
using DefaultOrgan = lkc::HotPatriciaOrgan;
using HotHull      = lkc::ObservableComposedContainer<DefaultOrgan>;

static_assert(lkc::HotPatriciaNodePool<DefaultStore>);
static_assert(std::is_same_v<typename DefaultStore::allocator_type, std::allocator<typename DefaultStore::node_type>>);
static_assert(requires(DefaultOrgan const& organ) { organ.store_allocator_statistics(); });
static_assert(requires(HotHull const& hull) { hull.store_allocator_statistics(); });

template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
struct PoolFlipComposition {
    using search_algo                          = SearchAlgoWrapper;
    using cache_traversal                      = comp::ArtComposition::cache_traversal;
    using mapping                              = comp::ArtComposition::mapping;
    using path_compression                     = comp::ArtComposition::path_compression;
    using node_type                            = comp::ArtComposition::node_type;
    using memory_layout                        = comp::ArtComposition::memory_layout;
    using allocator                            = comp::ArtComposition::allocator;
    using prefetch                             = comp::ArtComposition::prefetch;
    using concurrency                          = comp::ArtComposition::concurrency;
    using serialization                        = comp::ArtComposition::serialization;
    using telemetry                            = comp::ArtComposition::telemetry;
    using value_handle                         = comp::ArtComposition::value_handle;
    using isa                                  = comp::ArtComposition::isa;
    using index_organization                   = comp::ArtComposition::index_organization;
    using io_dispatch                          = comp::ArtComposition::io_dispatch;
    using migration_policy                     = comp::ArtComposition::migration_policy;
    using filter                               = comp::ArtComposition::filter;
    using queuing_q1                           = comp::ArtComposition::queuing_q1;
    using queuing_q2                           = comp::ArtComposition::queuing_q2;
    static constexpr std::string_view paper_id = "S7-6 HOT pool allocator DEG";
    static constexpr std::string_view name     = "S76HotPoolComposition";
};

[[nodiscard]] U64 value_for(U64 key) noexcept { return key ^ 0x9E3779B97F4A7C15ull; }

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

} // namespace

TEST(S76HotPoolAllocatorDeg, DirectPoolStatsAreReal) {
    HotHull                hull;
    std::vector<U64> const keys{40u, 12u, 90u, 7u, 25u, 70u, 110u, 1u, 3u, 6u, 9u, 13u, 32u, 48u, 64u};

    for (U64 const key : keys) { EXPECT_TRUE(hull.insert(key, value_for(key))); }

    auto const stats = hull.store_allocator_statistics();
    EXPECT_GT(stats.alloc_calls, 0u);
    EXPECT_GT(stats.bytes_allocated, 0u);
    EXPECT_GT(stats.live_nodes, 0u);
    EXPECT_GE(stats.live_nodes, keys.size());
}

TEST(S76HotPoolAllocatorDeg, AbiObserverRoutesPoolAllocatorStatsToT6) {
    using C       = PoolFlipComposition<lk::OriginalHotSearchAlgo>;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    auto  tier = std::make_unique<Adapter>();
    auto* drv  = static_cast<an::IDriveableTier*>(tier.get());
    auto* obs  = dynamic_cast<an::IObservableTier*>(drv);
    ASSERT_NE(obs, nullptr);

    std::vector<U64> const keys{32u, 16u, 48u, 8u, 24u, 40u, 56u, 4u, 12u, 20u, 28u, 36u, 44u, 52u, 60u};
    for (U64 const key : keys) { ASSERT_TRUE(drv->tier_insert(key, value_for(key))); }

    an::ComdareTierObserverSnapshot snap{};
    obs->tier_observe(&snap);

    std::cout << "S7-6 T6-DEG alloc_cnt=" << snap.axis_stats[6][2] << " bytes_alloc=" << snap.axis_stats[6][0]
              << " bytes_in_use=" << snap.axis_stats[6][1] << '\n';

    EXPECT_GT(snap.axis_stats[6][0], 0u);
    EXPECT_GT(snap.axis_stats[6][1], 0u);
    EXPECT_GT(snap.axis_stats[6][2], 0u);
}

TEST(S76HotPoolAllocatorDeg, DefaultStoreRoundtripIsUnchanged) {
    DefaultOrgan           organ;
    std::vector<U64> const keys{5u, 2u, 8u, 1u, 3u, 7u, 9u, 4u, 6u, 10u};

    for (U64 const key : keys) { organ.insert(key, value_for(key)); }

    EXPECT_EQ(organ.occupied_count(), keys.size());
    for (U64 const key : keys) {
        auto const found = organ.lookup(key);
        ASSERT_TRUE(found.has_value()) << "key=" << key;
        EXPECT_EQ(*found, value_for(key));
    }
    EXPECT_FALSE(organ.lookup(404u).has_value());

    organ.insert(3u, 333u);
    auto const updated = organ.lookup(3u);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(*updated, 333u);
    EXPECT_EQ(organ.occupied_count(), keys.size());
}

TEST(S76HotPoolAllocatorDeg, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 5);
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 5u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string>                     ids{"neutrality_guard"};
    std::vector<std::string>                     workloads{"ap9"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}
