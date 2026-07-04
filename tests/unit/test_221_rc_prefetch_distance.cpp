// #221 RC prefetch_distance: runtime RC knob reaches the real T7 store-prefetch path.
// The test keeps the binary/composition fixed, loads data once, then applies two RC distances
// and verifies the observer value axis_stats[7][6] comes from the real DistanceEstimator descent prefetch.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <topics/allocator/axis_06_allocator/axis_06_allocator_std_malloc.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_none.hpp>
#include <topics/filter/axis_filter/axis_filter_bloom.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_amd64.hpp>
#include <topics/io/axis_io/axis_io_in_memory_only.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/migration/axis_migration/axis_migration_none.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_distance_estimator.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_index_organized_table.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp>
#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace an    = ::comdare::cache_engine::anatomy;
namespace al    = ::comdare::cache_engine::allocator::axis_06_allocator;
namespace cc    = ::comdare::cache_engine::concurrency::axis_08_concurrency;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ct    = ::comdare::cache_engine::traversal::axis_03b_cache_traversal;
namespace flt   = ::comdare::cache_engine::filter::axis_filter;
namespace hw    = ::comdare::cache_engine::hardware::axis_09_isa;
namespace idx   = ::comdare::cache_engine::search_engine::axis_01_index_organization;
namespace ioax  = ::comdare::cache_engine::io::axis_io;
namespace map   = ::comdare::cache_engine::traversal::axis_03m_mapping;
namespace mig   = ::comdare::cache_engine::migration::axis_migration;
namespace ml    = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace nd    = ::comdare::cache_engine::nodes::axis_04_node_type;
namespace pc    = ::comdare::cache_engine::nodes::axis_02_path_compression;
namespace pf    = ::comdare::cache_engine::prefetch::axis_07_prefetch;
namespace q1    = ::comdare::cache_engine::queuing::axis_q1_queuing;
namespace q2    = ::comdare::cache_engine::queuing::axis_q2_queuing;
namespace ser   = ::comdare::cache_engine::serialization::axis_10_serialization;
namespace tel   = ::comdare::cache_engine::telemetry::axis_11_telemetry;
namespace vh    = ::comdare::cache_engine::value_handle::axis_14_value_handle;

namespace {

using DistanceStoreBackedComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, ct::LinearFanout, map::DirectPlacement, pc::PathCompressionNone,
    nd::ObservableNodeType<nd::Node256NodeType>,
    ml::ObservableMemoryLayout<ml::CacheLineAlignedMemoryLayout>, al::StdMalloc, pf::DistanceEstimatorPrefetch,
    cc::NoneConcurrency, ser::ObservableSerialization<ser::RawBinarySerialization>,
    tel::ObservableTelemetry<tel::LeafOnlyCounter>, vh::InlineValueHandle, hw::Amd64Isa, idx::IotIndexOrganization,
    ioax::InMemoryOnly, mig::NoMigration, flt::BloomFilter, q1::NoBuffer, q2::LazyFlush>;

using DistanceTier = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<DistanceStoreBackedComposition>>;

constexpr std::uint64_t kTestRecords = 128;
constexpr std::uint64_t kRcD1    = 2;
constexpr std::uint64_t kRcD2    = 9;

[[nodiscard]] constexpr std::uint64_t value_for(std::uint64_t key) noexcept { return key * 17u + 3u; }

void load_once(an::IObservableTier& tier) {
    tier.tier_clear();
    for (std::uint64_t key = 0; key < kTestRecords; ++key) {
        ASSERT_TRUE(tier.tier_insert(key, value_for(key)));
    }
    ASSERT_EQ(tier.tier_size(), kTestRecords);
}

[[nodiscard]] an::ComdareTierObserverSnapshot probe_after_lookups(an::IObservableTier& tier) {
    tier.tier_reset_statistics();
    for (std::uint64_t key = 0; key < kTestRecords; ++key) {
        std::uint64_t actual = 0;
        EXPECT_TRUE(tier.tier_lookup(key, &actual));
        EXPECT_EQ(actual, value_for(key));
    }

    an::ComdareTierObserverSnapshot snapshot{};
    tier.tier_observe(&snapshot);
    return snapshot;
}

void apply_prefetch_distance(an::IResourceControllableTier& rc, std::uint64_t distance) {
    an::ComdareResourceControlV1 control{};
    control.prefetch_distance = distance;
    EXPECT_EQ(rc.tier_apply_resource_control(&control), std::uint64_t{1});
}

} // namespace

TEST(RcPrefetchDistance221, RuntimeRcChangesRealT7DistanceOnSameLoadedTier) {
    DistanceTier tier{};
    auto*        base = static_cast<an::IAnatomyBase*>(&tier);
    auto*        obs  = dynamic_cast<an::IObservableTier*>(base);
    auto*        rc   = dynamic_cast<an::IResourceControllableTier*>(base);
    ASSERT_NE(obs, nullptr);
    ASSERT_NE(rc, nullptr);

    load_once(*obs);

    auto const default_snapshot = probe_after_lookups(*obs);
    auto const default_issued   = default_snapshot.axis_stats[7][5];
    auto const default_distance = default_snapshot.axis_stats[7][6];
    auto const expected_default = static_cast<std::uint64_t>(pf::DistanceEstimatorPrefetch::estimate(
        100.0 * static_cast<double>(kTestRecords) / static_cast<double>(kTestRecords + 1), 30.0));

    EXPECT_GT(default_issued, 0u);
    EXPECT_EQ(default_distance, expected_default);

    apply_prefetch_distance(*rc, kRcD1);
    auto const d1_snapshot = probe_after_lookups(*obs);
    EXPECT_GT(d1_snapshot.axis_stats[7][5], 0u);
    EXPECT_EQ(d1_snapshot.axis_stats[7][6], kRcD1);

    apply_prefetch_distance(*rc, kRcD2);
    auto const d2_snapshot = probe_after_lookups(*obs);
    EXPECT_GT(d2_snapshot.axis_stats[7][5], 0u);
    EXPECT_EQ(d2_snapshot.axis_stats[7][6], kRcD2);

    EXPECT_NE(d1_snapshot.axis_stats[7][6], d2_snapshot.axis_stats[7][6]);
    EXPECT_EQ(obs->tier_size(), kTestRecords);
}