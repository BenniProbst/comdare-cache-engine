// P33-VAMPIR Option A: VampirNfpAllocator conformance and neutral statistics check.

#include <axes/alloc/axis_06_allocator_vampir_nfp.hpp>
#include <cache_engine/concepts/cache_recommendation.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace alloc = ::comdare::cache_engine::alloc;

using VampirNfpAllocator = alloc::VampirNfpAllocator;

static_assert(alloc::concepts::AllocatorStrategy<VampirNfpAllocator>);
static_assert(alloc::concepts::CacheEnginePermutationStrategy<VampirNfpAllocator>);
static_assert(std::is_same_v<VampirNfpAllocator::axis_tag, alloc::subaxes::allocation_policy_tag>);
static_assert(VampirNfpAllocator::family_id::value == 24);
static_assert(VampirNfpAllocator::flag_suffix() == "VAMPIR_NFP");
static_assert(!VampirNfpAllocator::is_original_module());
static_assert(VampirNfpAllocator::get_compiler() == "original");

TEST(ComdareVampirNfpAllocator, StaticMetadataIsNeutralAndFaithful) {
    EXPECT_EQ(VampirNfpAllocator::name(), std::string_view{"vampir_nfp"});
    EXPECT_EQ(VampirNfpAllocator::family_name(), std::string_view{"VAMPIR NFP memory-type virtualization allocator"});
    EXPECT_TRUE(VampirNfpAllocator::is_thread_safe());
    EXPECT_TRUE(VampirNfpAllocator::supports_pmr());
    EXPECT_FALSE(VampirNfpAllocator::requires_explicit_init());
    EXPECT_FALSE(VampirNfpAllocator::supports_numa_node_hint());
    EXPECT_FALSE(VampirNfpAllocator::supports_thread_local_cache());
    EXPECT_FALSE(VampirNfpAllocator::requires_specialized_hardware());
    EXPECT_EQ(VampirNfpAllocator::progress_guarantee(), alloc::concepts::ProgressGuarantee::Blocking);
    EXPECT_EQ(VampirNfpAllocator::target_tier(), ::comdare::cache_engine::MemoryAllocationHint::TierKind::Dram);

    constexpr auto nfp = VampirNfpAllocator::nfp_descriptor();
    EXPECT_EQ(nfp.bandwidth_class, std::string_view{"unspecified"});
    EXPECT_EQ(nfp.latency_class, std::string_view{"unspecified"});
    EXPECT_EQ(nfp.capacity, 0u);
}

TEST(ComdareVampirNfpAllocator, AllocateDeallocateAndResetStatistics) {
    VampirNfpAllocator a{};
    VampirNfpAllocator b{};
    EXPECT_TRUE(a == b);

    constexpr std::size_t bytes     = 100u;
    constexpr std::size_t alignment = 64u;
    void*                 p         = a.allocate(bytes, alignment);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0u);
    std::memset(p, 0xA5, bytes);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto stats = a.statistics();
    EXPECT_EQ(stats.allocation_count, 1u);
    EXPECT_EQ(stats.deallocation_count, 0u);
    EXPECT_EQ(stats.failure_count, 0u);
    EXPECT_EQ(stats.total_bytes_allocated, 128u);
    EXPECT_EQ(stats.total_bytes_in_use, 128u);
#endif

    a.deallocate(p, bytes, alignment);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    stats = a.snapshot();
    EXPECT_EQ(stats.allocation_count, 1u);
    EXPECT_EQ(stats.deallocation_count, 1u);
    EXPECT_EQ(stats.total_bytes_allocated, 128u);
    EXPECT_EQ(stats.total_bytes_in_use, 0u);

    a.reset();
    stats = a.statistics();
    EXPECT_EQ(stats.allocation_count, 0u);
    EXPECT_EQ(stats.deallocation_count, 0u);
    EXPECT_EQ(stats.failure_count, 0u);
    EXPECT_EQ(stats.total_bytes_allocated, 0u);
    EXPECT_EQ(stats.total_bytes_in_use, 0u);
#endif
}