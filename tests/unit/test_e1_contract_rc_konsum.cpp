// Schicht E1: Laufzeit-RC wird von T6/T8/T1/T11 ehrlich konsumiert.
// Keine DLL noetig: der SearchAlgorithmAbiAdapter wird in-process instanziiert.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include <axes/concurrency_axis/axis_08_concurrency_observable.hpp>
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
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_versioned_pointer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <barrier>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace an    = ::comdare::cache_engine::anatomy;
namespace al    = ::comdare::cache_engine::allocator::axis_06_allocator;
namespace cc    = ::comdare::cache_engine::concurrency::axis_08_concurrency;
namespace ccobs = ::comdare::cache_engine::concurrency_axis;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ct    = ::comdare::cache_engine::traversal::axis_03b_cache_traversal;
namespace ex    = ::comdare::cache_engine::builder::experiment;
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
namespace vh    = ::comdare::cache_engine::value_handle::axis_14_value_handle;

namespace {

template <class ValueHandle = vh::InlineValueHandle>
using RcComposition =
    an::AdHocComposition<ce03a::Array256SearchAlgo, ct::LinearFanout, map::DirectPlacement, pc::PathCompressionNone,
                         nd::ObservableNodeType<nd::Node4NodeType>,
                         ml::ObservableMemoryLayout<ml::CacheLineAlignedMemoryLayout>, al::StdMalloc, pf::NonePrefetch,
                         cc::BlockingConcurrency, ser::ObservableSerialization<ser::RawBinarySerialization>,
                         ValueHandle, idx::IotIndexOrganization, ioax::InMemoryOnly, mig::NoMigration, flt::BloomFilter,
                         q1::NoBuffer, q2::LazyFlush>; // INC-2d: isa raus (17 Slots)

template <class ValueHandle = vh::InlineValueHandle>
using RcTier = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<RcComposition<ValueHandle>>>;

constexpr std::uint64_t kChunkBudget = 4u * 64u;

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

void apply_rc(an::IResourceControllableTier& rc, an::ComdareResourceControlV1 control, std::uint64_t expected) {
    EXPECT_EQ(rc.tier_apply_resource_control(&control), expected);
}

[[nodiscard]] an::ComdareTierObserverSnapshot observe(an::IObservableTier& tier) {
    an::ComdareTierObserverSnapshot snapshot{};
    tier.tier_observe(&snapshot);
    return snapshot;
}

void insert_range(an::IObservableTier& tier, std::uint64_t n) {
    for (std::uint64_t key = 0; key < n; ++key) { ASSERT_TRUE(tier.tier_insert(key, value_for(key))); }
    ASSERT_EQ(tier.tier_size(), n);
}

[[nodiscard]] std::vector<std::string> split_semicolon(std::string const& s) {
    std::vector<std::string> out;
    std::size_t              start = 0;
    while (true) {
        std::size_t const pos = s.find(';', start);
        if (pos == std::string::npos) {
            out.push_back(s.substr(start));
            return out;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

[[nodiscard]] std::size_t column_index(std::vector<std::string> const& header, std::string const& name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return i;
    return header.size();
}

} // namespace

TEST(E1ResourceControlKonsum, ReusedTierZeroResetsBatchAndPoolBudget) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);
    insert_range(*tv.obs, 8u);

    an::ComdareResourceControlV1 batch{};
    batch.batch_size = 999u;
    apply_rc(*tv.rc, batch, 1u);
    EXPECT_EQ(observe(*tv.obs).axis_stats[1][6], 999u);

    an::ComdareResourceControlV1 reset{};
    apply_rc(*tv.rc, reset, 0u);
    EXPECT_EQ(observe(*tv.obs).axis_stats[1][6], 256u);

    an::ComdareResourceControlV1 budget{};
    budget.pool_budget_bytes = kChunkBudget;
    apply_rc(*tv.rc, budget, 1u);
    tv.obs->tier_clear();
    insert_range(*tv.obs, 4u);
    EXPECT_FALSE(tv.obs->tier_insert(4u, value_for(4u)));

    apply_rc(*tv.rc, reset, 0u);
    tv.obs->tier_clear();
    insert_range(*tv.obs, 8u);
}

TEST(E1ResourceControlKonsum, PoolBudgetRejectDoesNotBreakStoreInvariants) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);

    an::ComdareResourceControlV1 budget{};
    budget.pool_budget_bytes = kChunkBudget;
    apply_rc(*tv.rc, budget, 1u);

    insert_range(*tv.obs, 4u);
    EXPECT_FALSE(tv.obs->tier_insert(4u, value_for(4u)));
    EXPECT_EQ(tv.obs->tier_size(), 4u);
    EXPECT_GT(observe(*tv.obs).axis_stats[6][5], 0u);

    EXPECT_TRUE(tv.obs->tier_erase(1u));
    for (std::uint64_t key : {0u, 2u, 3u}) {
        std::uint64_t actual = 0;
        EXPECT_TRUE(tv.obs->tier_lookup(key, &actual));
        EXPECT_EQ(actual, value_for(key));
    }

    tv.obs->tier_clear();
    EXPECT_EQ(observe(*tv.obs).axis_stats[6][5], 0u);
}

TEST(E1ResourceControlKonsum, RejectedInsertDoesNotDriveObserverAuxOrgans) {
    RcTier<> tier{};
    auto     tv = views(tier);
    ASSERT_NE(tv.obs, nullptr);
    ASSERT_NE(tv.rc, nullptr);

    an::ComdareResourceControlV1 budget{};
    budget.pool_budget_bytes = kChunkBudget;
    apply_rc(*tv.rc, budget, 1u);
    insert_range(*tv.obs, 4u);
    tv.obs->tier_reset_statistics();

    EXPECT_FALSE(tv.obs->tier_insert(4u, value_for(4u)));
    auto const snapshot = observe(*tv.obs);

    EXPECT_EQ(snapshot.axis_stats[0][3], 0u);
    EXPECT_EQ(snapshot.axis_stats[10][0], 4u);
    // Bau-INC-2d: filter ist von Slot 15 auf 14 gerueckt (isa raus, Indizes >=11 -1).
    EXPECT_EQ(snapshot.axis_stats[14][1] + snapshot.axis_stats[14][2], 4u);
    EXPECT_EQ(tv.obs->tier_size(), 4u);
}

TEST(E1ResourceControlKonsum, BlockingConcurrencyReportsOnlyRealContention) {
    using Organ = ccobs::ObservableConcurrency<cc::BlockingConcurrency>;

    Organ single{};
    single.acquire();
    single.release();
    EXPECT_EQ(single.statistics().contention_count, 0u);

    cc::BlockingConcurrency::acquire();
    constexpr std::size_t               kThreads = 4;
    std::barrier<>                      start(static_cast<std::ptrdiff_t>(kThreads + 1u));
    std::array<std::uint64_t, kThreads> contentions{};
    std::vector<std::jthread>           threads;
    threads.reserve(kThreads);
    for (std::size_t i = 0; i < kThreads; ++i) {
        threads.emplace_back([&start, &contentions, i] {
            Organ contender{};
            start.arrive_and_wait();
            contender.acquire();
            contender.release();
            contentions[i] = contender.statistics().contention_count;
        });
    }

    start.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    cc::BlockingConcurrency::release();
    threads.clear();

    std::uint64_t total_contention = 0;
    for (auto c : contentions) total_contention += c;
    EXPECT_GT(total_contention, 0u);
}

TEST(E1ResourceControlKonsum, InlineThresholdOverrideCoexistsWithValueHandleAccounting) {
    RcTier<> inline_tier{};
    auto     inline_tv = views(inline_tier);
    ASSERT_NE(inline_tv.obs, nullptr);
    ASSERT_NE(inline_tv.rc, nullptr);
    insert_range(*inline_tv.obs, 8u);

    auto const inline_default = observe(*inline_tv.obs);
    EXPECT_EQ(inline_default.axis_stats[10][1], 0u);
    EXPECT_EQ(inline_default.axis_stats[10][3], 1u);

    an::ComdareResourceControlV1 external{};
    external.inline_threshold_bytes = 1u;
    apply_rc(*inline_tv.rc, external, 1u);
    auto const inline_external = observe(*inline_tv.obs);
    EXPECT_GT(inline_external.axis_stats[10][1], 0u);
    EXPECT_EQ(inline_external.axis_stats[10][3], 2u);

    an::ComdareResourceControlV1 reset{};
    apply_rc(*inline_tv.rc, reset, 0u);
    auto const inline_reset = observe(*inline_tv.obs);
    EXPECT_EQ(inline_reset.axis_stats[10][1], 0u);
    EXPECT_EQ(inline_reset.axis_stats[10][3], 1u);

    RcTier<vh::VersionedPointerValueHandle> versioned_tier{};
    auto                                    versioned_tv = views(versioned_tier);
    ASSERT_NE(versioned_tv.obs, nullptr);
    ASSERT_NE(versioned_tv.rc, nullptr);
    insert_range(*versioned_tv.obs, 8u);
    apply_rc(*versioned_tv.rc, external, 1u);
    auto const versioned_external = observe(*versioned_tv.obs);
    EXPECT_GT(versioned_external.axis_stats[10][1], 0u);
    EXPECT_GT(versioned_external.axis_stats[10][2], 0u);
    EXPECT_GE(versioned_external.axis_stats[10][3], 2u);

    apply_rc(*versioned_tv.rc, reset, 0u);
    auto const versioned_default = observe(*versioned_tv.obs);
    EXPECT_GT(versioned_default.axis_stats[10][1], 0u);
    EXPECT_GT(versioned_default.axis_stats[10][2], 0u);
    EXPECT_GE(versioned_default.axis_stats[10][3], 2u);
}

TEST(E1ResourceControlKonsum, CsvSchemaNamesAndRowsCarryNewRcEvidence) {
    std::string const header_text = ex::lazy_csv_header();
    EXPECT_NE(header_text.find("stat_cache_traversal_batch_size"), std::string::npos);
    EXPECT_NE(header_text.find("stat_allocator_budget_reject"), std::string::npos);

    ex::LazyMeasuredRow row{};
    row.binary_id                     = "e1";
    row.setting_label                 = "rc";
    row.n_ops                         = 1u;
    row.timed_ops                     = 1u;
    row.total_ns                      = 10;
    row.unified_real                  = true;
    row.unified.axis_stats[1][6]      = 777u;
    row.unified.axis_stats[6][5]      = 3u;
    row.unified.filled_axis_count     = an::kV3FilledAxisCount;
    row.unified.observable_axis_count = an::kV3FilledAxisCount;
    row.unified.tier_fill_level       = 4u;
    row.unified.seg_run_total_ns      = 1;
    row.unified.seg_framework_ns      = 0;
    row.applied_axis_count            = 2u;

    auto const header = split_semicolon(header_text);
    auto const fields = split_semicolon(ex::format_csv_row(row));
    auto const batch  = column_index(header, "stat_cache_traversal_batch_size");
    auto const reject = column_index(header, "stat_allocator_budget_reject");
    ASSERT_LT(batch, header.size());
    ASSERT_LT(reject, header.size());
    ASSERT_LT(batch, fields.size());
    ASSERT_LT(reject, fields.size());
    EXPECT_EQ(fields[batch], "777");
    EXPECT_EQ(fields[reject], "3");
}
