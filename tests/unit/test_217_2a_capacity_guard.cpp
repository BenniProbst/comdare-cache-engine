// #217-2a (2026-07-03) -- harte Static-Container-Kapazitaet ehrlich in tier_insert ablehnen.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_concept.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/lookup/axis_03a_search_algo_array256.hpp>
#include <axes/lookup/axis_03a_search_algo_array65535.hpp>
#include <axes/lookup/axis_03a_search_algo_linear_scan.hpp>
#include <axes/lookup/composable/capacity_constraint.hpp>
#include <compositions/art_reference.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace sa   = ::comdare::cache_engine::lookup;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

template <class SearchAlgo>
using StoreBackedAdHocComposition = an::AdHocComposition<
    SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::value_handle, comp::ArtComposition::index_organization,
    comp::ArtComposition::io_dispatch, comp::ArtComposition::migration_policy, comp::ArtComposition::filter,
    comp::ArtComposition::queuing_q1, comp::ArtComposition::queuing_q2>;

template <class SearchAlgo>
using AdapterFor = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<StoreBackedAdHocComposition<SearchAlgo>>>;

[[nodiscard]] constexpr std::uint64_t value_for(std::uint64_t key) noexcept { return key ^ 0x9E3779B97F4A7C15ull; }

template <class SearchAlgo>
[[nodiscard]] an::IDriveableTier& drive(AdapterFor<SearchAlgo>& adapter) noexcept {
    return static_cast<an::IDriveableTier&>(adapter);
}

} // namespace

TEST(CapacityConstraint2172a, TraitReportsStaticArraysAndAdvisoryLinearScan) {
    constexpr auto array256 = lkc::capacity_constraint_of<sa::Array256SearchAlgo>();
    static_assert(array256.kind == lkc::CapacityKind::Static);
    static_assert(array256.max_size == 256u);
    static_assert(array256.min_fill == 0u);

    constexpr auto array65535 = lkc::capacity_constraint_of<sa::Array65535SearchAlgo>();
    static_assert(array65535.kind == lkc::CapacityKind::Static);
    static_assert(array65535.max_size == sa::Array65535SearchAlgo::kCapacity);
    static_assert(array65535.min_fill == 0u);

    constexpr auto linear = lkc::capacity_constraint_of<sa::LinearScanSearchAlgo>();
    static_assert(linear.kind == lkc::CapacityKind::Advisory);
    static_assert(linear.max_size == 0u);

    SUCCEED();
}

TEST(CapacityGuard2172a, Array65535AcceptsLastSlotAndRejectsOutOfCapacityKeys) {
    AdapterFor<sa::Array65535SearchAlgo> adapter{};
    auto&                                tier = drive(adapter);

    constexpr std::uint64_t in_range = 65'535u;
    EXPECT_TRUE(tier.tier_insert(in_range, value_for(in_range)));
    EXPECT_EQ(tier.tier_size(), 1u);

    std::uint64_t actual = 0;
    EXPECT_TRUE(tier.tier_lookup(in_range, &actual));
    EXPECT_EQ(actual, value_for(in_range));

    EXPECT_FALSE(tier.tier_insert(65'536u, value_for(65'536u)));
    EXPECT_EQ(tier.tier_size(), 1u);
    EXPECT_FALSE(tier.tier_insert(1'000'000'000u, value_for(1'000'000'000u)));
    EXPECT_EQ(tier.tier_size(), 1u);
}

TEST(CapacityGuard2172a, Array256AcceptsLastSlotAndRejectsOutOfCapacityKey) {
    AdapterFor<sa::Array256SearchAlgo> adapter{};
    auto&                              tier = drive(adapter);

    constexpr std::uint64_t in_range = 255u;
    EXPECT_TRUE(tier.tier_insert(in_range, value_for(in_range)));
    EXPECT_EQ(tier.tier_size(), 1u);

    std::uint64_t actual = 0;
    EXPECT_TRUE(tier.tier_lookup(in_range, &actual));
    EXPECT_EQ(actual, value_for(in_range));

    EXPECT_FALSE(tier.tier_insert(256u, value_for(256u)));
    EXPECT_EQ(tier.tier_size(), 1u);
}

TEST(CapacityGuard2172a, AdvisoryLinearScanStillAcceptsWideKey) {
    AdapterFor<sa::LinearScanSearchAlgo> adapter{};
    auto&                                tier = drive(adapter);

    constexpr std::uint64_t wide_key = 1'000'000'000u;
    EXPECT_TRUE(tier.tier_insert(wide_key, value_for(wide_key)));
    EXPECT_EQ(tier.tier_size(), 1u);

    std::uint64_t actual = 0;
    EXPECT_TRUE(tier.tier_lookup(wide_key, &actual));
    EXPECT_EQ(actual, value_for(wide_key));
}