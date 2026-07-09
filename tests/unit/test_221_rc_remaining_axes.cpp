// #221 RC remaining axes: thread_count, pool_budget_bytes, batch_size and inline_threshold_bytes
// reach the real organs at runtime. Each test keeps the composition fixed and varies only RC.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <axes/node/axis_04_node_type_observable.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_std_malloc.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_blocking.hpp>
#include <topics/filter/axis_filter/axis_filter_bloom.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_amd64.hpp>
#include <topics/io/axis_io/axis_io_in_memory_only.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/migration/axis_migration/axis_migration_none.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node4.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp>
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

template <class Concurrency = cc::BlockingConcurrency, class ValueHandle = vh::InlineValueHandle>
using RcComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, ct::LinearFanout, map::DirectPlacement, pc::PathCompressionNone,
    nd::ObservableNodeType<nd::Node4NodeType>, ml::ObservableMemoryLayout<ml::CacheLineAlignedMemoryLayout>,
    al::StdMalloc, pf::NonePrefetch, Concurrency, ser::ObservableSerialization<ser::RawBinarySerialization>,
    tel::ObservableTelemetry<tel::LeafOnlyCounter>, ValueHandle, hw::Amd64Isa, idx::IotIndexOrganization,
    ioax::InMemoryOnly, mig::NoMigration, flt::BloomFilter, q1::NoBuffer, q2::LazyFlush>;

template <class Concurrency = cc::BlockingConcurrency, class ValueHandle = vh::InlineValueHandle>
using RcTier = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<RcComposition<Concurrency, ValueHandle>>>;

constexpr std::uint64_t kRecords       = 16;
constexpr std::uint64_t kNode4ClaChunk = 4u * 64u;

[[nodiscard]] constexpr std::uint64_t value_for(std::uint64_t key) noexcept { return key * 19u + 5u; }

struct TierViews {
    an::IObservableTier*           obs = nullptr;
    an::IResourceControllableTier* rc  = nullptr;
};

template <class Tier>
[[nodiscard]] TierViews views(Tier& tier) {
    auto* base = static_cast<an::IAnatomyBase*>(&tier);
    auto* obs  = dynamic_cast<an::IObservableTier*>(base);
    auto* rc   = dynamic_cast<an::IResourceControllableTier*>(base);
    EXPECT_NE(obs, nullptr);
    EXPECT_NE(rc, nullptr);
    return TierViews{obs, rc};
}

void load_records(an::IObservableTier& tier, std::uint64_t n = kRecords) {
    tier.tier_clear();
    for (std::uint64_t key = 0; key < n; ++key) { ASSERT_TRUE(tier.tier_insert(key, value_for(key))); }
    ASSERT_EQ(tier.tier_size(), n);
    tier.tier_reset_statistics();
}

[[nodiscard]] an::ComdareTierObserverSnapshot observe(an::IObservableTier& tier) {
    an::ComdareTierObserverSnapshot snapshot{};
    tier.tier_observe(&snapshot);
    return snapshot;
}

void apply_rc(an::IResourceControllableTier& rc, an::ComdareResourceControlV1 control) {
    EXPECT_EQ(rc.tier_apply_resource_control(&control), std::uint64_t{1});
}

void lookup_round(an::IObservableTier& tier, std::uint64_t n = kRecords) {
    tier.tier_reset_statistics();
    for (std::uint64_t key = 0; key < n; ++key) {
        std::uint64_t actual = 0;
        EXPECT_TRUE(tier.tier_lookup(key, &actual));
        EXPECT_EQ(actual, value_for(key));
    }
}

} // namespace

TEST(RcThreadCount221, RuntimeRcScalesRealT8CriticalSections) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);
    load_records(*tv.obs);

    lookup_round(*tv.obs);
    auto const default_snapshot = observe(*tv.obs);
    EXPECT_EQ(default_snapshot.axis_stats[8][0], kRecords);
    EXPECT_EQ(default_snapshot.axis_stats[8][1], kRecords);

    an::ComdareResourceControlV1 control{};
    control.thread_count = 3;
    apply_rc(*tv.rc, control);

    lookup_round(*tv.obs);
    auto const rc_snapshot = observe(*tv.obs);
    EXPECT_EQ(rc_snapshot.axis_stats[8][0], kRecords * 3u);
    EXPECT_EQ(rc_snapshot.axis_stats[8][1], kRecords * 3u);
}

TEST(RcPoolBudget221, RuntimeRcRejectsGrowthBeyondRealAllocatorBudget) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);

    an::ComdareResourceControlV1 zero{};
    EXPECT_EQ(tv.rc->tier_apply_resource_control(&zero), std::uint64_t{0});
    for (std::uint64_t key = 0; key < 5; ++key) { ASSERT_TRUE(tv.obs->tier_insert(key, value_for(key))); }
    EXPECT_EQ(tv.obs->tier_size(), 5u);

    tv.obs->tier_clear();
    an::ComdareResourceControlV1 control{};
    control.pool_budget_bytes = kNode4ClaChunk;
    apply_rc(*tv.rc, control);

    for (std::uint64_t key = 0; key < 4; ++key) { ASSERT_TRUE(tv.obs->tier_insert(key, value_for(key))); }
    EXPECT_EQ(tv.obs->tier_size(), 4u);
    EXPECT_FALSE(tv.obs->tier_insert(4u, value_for(4u)));
    EXPECT_EQ(tv.obs->tier_size(), 4u);

    auto const snapshot = observe(*tv.obs);
    EXPECT_EQ(snapshot.axis_stats[6][1], kNode4ClaChunk);
    EXPECT_GT(snapshot.axis_stats[6][4], 0u);
}

TEST(RcBatchSize221, RuntimeRcDrivesRealTraversalBatchProbeOnSameTier) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);
    load_records(*tv.obs);

    lookup_round(*tv.obs, 3u);
    auto const default_snapshot = observe(*tv.obs);
    EXPECT_EQ(default_snapshot.axis_stats[0][6], 0u);
    EXPECT_EQ(default_snapshot.axis_stats[0][7], 0u);

    an::ComdareResourceControlV1 control{};
    control.batch_size = 5;
    apply_rc(*tv.rc, control);

    lookup_round(*tv.obs, 3u);
    auto const rc_snapshot = observe(*tv.obs);
    EXPECT_EQ(rc_snapshot.axis_stats[0][6], 5u);
    EXPECT_EQ(rc_snapshot.axis_stats[0][7], 15u);
    EXPECT_EQ(rc_snapshot.axis_stats[0][0], 3u);
}

TEST(RcInlineThreshold221, RuntimeRcSwitchesValueHandleInlineVsExternalPath) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);
    load_records(*tv.obs);

    auto const default_snapshot = observe(*tv.obs);
    EXPECT_EQ(default_snapshot.axis_stats[11][0], kRecords);
    EXPECT_EQ(default_snapshot.axis_stats[11][1], 0u);
    EXPECT_EQ(default_snapshot.axis_stats[11][3], 1u);

    an::ComdareResourceControlV1 external_control{};
    external_control.inline_threshold_bytes = 4;
    apply_rc(*tv.rc, external_control);
    auto const external_snapshot = observe(*tv.obs);
    EXPECT_EQ(external_snapshot.axis_stats[11][0], kRecords);
    EXPECT_EQ(external_snapshot.axis_stats[11][1], kRecords);
    EXPECT_EQ(external_snapshot.axis_stats[11][3], 2u);

    an::ComdareResourceControlV1 inline_control{};
    inline_control.inline_threshold_bytes = 16;
    apply_rc(*tv.rc, inline_control);
    auto const inline_snapshot = observe(*tv.obs);
    EXPECT_EQ(inline_snapshot.axis_stats[11][0], kRecords);
    EXPECT_EQ(inline_snapshot.axis_stats[11][1], 0u);
    EXPECT_EQ(inline_snapshot.axis_stats[11][3], 1u);
}
