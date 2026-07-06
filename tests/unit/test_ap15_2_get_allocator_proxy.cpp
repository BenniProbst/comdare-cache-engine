// AP15-2 (#263-Rest, Strang C) -- get_allocator als POD-Proxy-Sub-Interface.

#include "builder/measurement_snapshot.hpp"
#include "builder/pruef_dock/conformance_gate.hpp"

#include <anatomy/abi_adapter.hpp>
#include <anatomy/allocator_proxy_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/lookup/axis_03a_search_algo_bst.hpp>
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
namespace pd   = ::comdare::cache_engine::builder::pruef_dock;
namespace comp = ::comdare::cache_engine::compositions;
namespace lk   = ::comdare::cache_engine::lookup;

namespace {

using U64 = std::uint64_t;

template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. namespace = interne Bindung je TU
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
    static constexpr std::string_view paper_id = "AP15-2 allocator proxy";
    static constexpr std::string_view name     = "AP152AllocatorProxyComposition";
};

class PlainDriveableTier final : public an::IDriveableTier {
public:
    bool          tier_insert(std::uint64_t, std::uint64_t) noexcept override { return false; }
    bool          tier_lookup(std::uint64_t, std::uint64_t*) const noexcept override { return false; }
    bool          tier_erase(std::uint64_t) noexcept override { return false; }
    void          tier_clear() noexcept override {}
    std::uint64_t tier_size() const noexcept override { return 0; }
};

[[nodiscard]] U64 value_for(U64 key) noexcept { return key ^ 0x9E3779B97F4A7C15ull; }

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

} // namespace

TEST(AP152GetAllocatorProxy, AdapterProxyIsReal) {
    using C       = PoolFlipComposition<lk::BinarySearchTreeSearchAlgo>;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    auto  tier = std::make_unique<Adapter>();
    auto* drv  = static_cast<an::IDriveableTier*>(tier.get());

    std::vector<U64> const keys{32u, 16u, 48u, 8u, 24u, 40u, 56u, 4u, 12u};
    for (U64 const key : keys) { ASSERT_TRUE(drv->tier_insert(key, value_for(key))); }

    auto* alloc = dynamic_cast<an::IAllocatorProxyTier*>(drv);
    ASSERT_NE(alloc, nullptr);

    an::ComdareAllocatorProxyV1 proxy{};
    alloc->tier_get_allocator(&proxy);

    EXPECT_EQ(proxy.format_version, an::kAllocatorProxyFormatVersion);
    EXPECT_NE(proxy.flags & (U64{1} << 0u), 0u);
    EXPECT_EQ(proxy.allocator_family_id, static_cast<U64>(C::allocator::family_id::value));
    EXPECT_NE(proxy.flags & (U64{1} << 1u), 0u);
    EXPECT_GT(proxy.total_bytes_allocated, 0u);
    EXPECT_GT(proxy.total_bytes_in_use, 0u);
    EXPECT_GT(proxy.allocation_count, 0u);
    EXPECT_GT(proxy.live_nodes, 0u);

    std::cout << "AP15-2 get_allocator proxy family=" << proxy.allocator_family_id
              << " bytes=" << proxy.total_bytes_allocated << " allocs=" << proxy.allocation_count << '\n';
}

TEST(AP152GetAllocatorProxy, HonestSkipOnPlainTier) {
    PlainDriveableTier tier;
    auto const         result = pd::probe_allocator_proxy(tier);

    EXPECT_FALSE(result.interface_present);
    EXPECT_FALSE(result.identity_present);
    EXPECT_FALSE(result.stats_route_present);
    EXPECT_EQ(result.proxy.flags, 0u);
}

TEST(AP152GetAllocatorProxy, GateProbeOnAdapter) {
    using C       = PoolFlipComposition<lk::BinarySearchTreeSearchAlgo>;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    auto  tier = std::make_unique<Adapter>();
    auto* drv  = static_cast<an::IDriveableTier*>(tier.get());
    for (U64 const key : std::vector<U64>{7u, 3u, 11u, 1u}) { ASSERT_TRUE(drv->tier_insert(key, value_for(key))); }

    auto const result = pd::probe_allocator_proxy(*drv);
    EXPECT_TRUE(result.interface_present);
    EXPECT_TRUE(result.identity_present);
    EXPECT_TRUE(result.stats_route_present);
}

TEST(AP152GetAllocatorProxy, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 4);
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 5u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string>                     ids{"neutrality_guard"};
    std::vector<std::string>                     workloads{"ap15_2"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}
