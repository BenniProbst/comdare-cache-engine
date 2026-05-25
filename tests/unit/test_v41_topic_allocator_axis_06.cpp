// V41.F.6.1.A Test fuer Topic Allocator Achse 6 (W1-revidiert, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Tests:
// (1) Concept-Konformanz (compile-time)
//     - AllocatorComponent (Topic-Marker)
//     - AllocatorStrategy (Pflicht-Standard)
//     - CacheEnginePermutationStrategy (cache-engine-spec)
//     - ZeroingStrategy / ReallocatingStrategy (Sub-Concepts erfuellt)
//     - NICHT ResettableStrategy / IntrospectableStrategy / etc. (nicht erfuellt — Dummy ohne)
// (2) Compile-Time-Eigenschaften (Pflicht-API-Gruppen 2)
// (3) Runtime: allocate/deallocate roundtrip
// (4) Runtime: zero_allocate + reallocate
// (5) CRTP-Delegate

#include <topics/allocator/concepts/topic_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_zeroing_strategy_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_overallocating_strategy_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_introspectable_strategy_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_reclaimable_strategy_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_resettable_strategy_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_reallocating_strategy_concept.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_subaxes_aa1_to_aa7.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_strategy_base.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_std_malloc.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace topic_alloc       = comdare::cache_engine::allocator;
namespace topic_alloc_cpts  = comdare::cache_engine::allocator::concepts;
namespace axis_06           = comdare::cache_engine::allocator::axis_06_allocator;
namespace axis_06_cpts      = comdare::cache_engine::allocator::axis_06_allocator::concepts;
namespace subaxes           = comdare::cache_engine::allocator::axis_06_allocator::subaxes;

// ─────────────────────────────────────────────────────────────────────────────
// (1) Concept-Konformanz Beweise
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesTopicConcept) {
    static_assert(topic_alloc_cpts::AllocatorComponent<axis_06::StdMalloc>);
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesAllocatorStrategy) {
    static_assert(axis_06_cpts::AllocatorStrategy<axis_06::StdMalloc>,
        "Pflicht-Standard: allocate/deallocate/value_type/size_type/operator==");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesCacheEnginePermutationStrategy) {
    static_assert(axis_06_cpts::CacheEnginePermutationStrategy<axis_06::StdMalloc>,
        "Pflicht cache-engine-spec: axis_tag/family_id/name/.../statistics");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesZeroingStrategy) {
    static_assert(axis_06_cpts::ZeroingStrategy<axis_06::StdMalloc>,
        "Optional: StdMalloc bietet zero_allocate (calloc)");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesReallocatingStrategy) {
    static_assert(axis_06_cpts::ReallocatingStrategy<axis_06::StdMalloc>,
        "Optional: StdMalloc bietet reallocate (realloc)");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocDoesNotSatisfyPoolResettableStrategy) {
    // libc malloc kennt kein release_all() (Pool/Arena-spezifisch) -> Sub-Concept NICHT erfuellt
    static_assert(!axis_06_cpts::PoolResettableStrategy<axis_06::StdMalloc>,
        "Negativ: libc malloc hat kein release_all() (Pool/Arena Sub-Concept)");
    SUCCEED();
}

#ifdef COMDARE_CE_ENABLE_STATISTICS
TEST(V41_TopicAllocatorAxis06, ResetClearsStatistics) {
    // V41.F.6.1.A User-Klarstellung: reset() ist Statistik-Reset (NICHT Pool-Reset!)
    axis_06::StdMalloc m{};
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);
    m.deallocate(p, 64, 8);
    EXPECT_EQ(m.statistics().allocation_count, 1u);

    m.reset();   // = Statistik-Reset
    auto stats = m.statistics();
    EXPECT_EQ(stats.allocation_count,   0u);
    EXPECT_EQ(stats.deallocation_count, 0u);
    EXPECT_EQ(stats.total_bytes_in_use, 0u);
}
#endif

TEST(V41_TopicAllocatorAxis06, StdMallocDoesNotSatisfyIntrospectableStrategy) {
    static_assert(!axis_06_cpts::IntrospectableStrategy<axis_06::StdMalloc>,
        "Negativ: libc malloc hat kein portables usable_size");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocDoesNotSatisfyOverAllocatingStrategy) {
    static_assert(!axis_06_cpts::OverAllocatingStrategy<axis_06::StdMalloc>,
        "Negativ: libc malloc hat kein allocate_at_least");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocDoesNotSatisfyReclaimableStrategy) {
    static_assert(!axis_06_cpts::ReclaimableStrategy<axis_06::StdMalloc>,
        "Negativ: libc malloc hat kein portables collect/trim");
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Negative-Beweise: Dummy ohne Concept-API
// ─────────────────────────────────────────────────────────────────────────────

struct DummyNotAllocator {};   // weder topic_tag noch allocate

TEST(V41_TopicAllocatorAxis06, DummyDoesNotSatisfyAnyConcept) {
    static_assert(!topic_alloc_cpts::AllocatorComponent<DummyNotAllocator>);
    static_assert(!axis_06_cpts::AllocatorStrategy<DummyNotAllocator>);
    static_assert(!axis_06_cpts::CacheEnginePermutationStrategy<DummyNotAllocator>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (2) Compile-Time-Eigenschaften
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, StdMallocCompileTimeProperties) {
    using SM = axis_06::StdMalloc;
    static_assert(std::same_as<SM::topic_tag,   topic_alloc_cpts::AllocatorTopicTag>);
    static_assert(std::same_as<SM::axis_tag,    subaxes::size_class_schema_tag>);
    static_assert(std::same_as<SM::value_type,  std::byte>);
    static_assert(std::same_as<SM::size_type,   std::size_t>);
    static_assert(SM::family_id::value == 22);
    static_assert(SM::is_thread_safe());
    static_assert(SM::supports_pmr());
    static_assert(SM::max_alignment() >= alignof(std::max_align_t));
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocIdentification) {
    using SM = axis_06::StdMalloc;
    EXPECT_EQ(SM::name(),        "std_malloc");
    EXPECT_FALSE(SM::family_name().empty());
}

TEST(V41_TopicAllocatorAxis06, StdMallocEquality) {
    axis_06::StdMalloc a{};
    axis_06::StdMalloc b{};
    EXPECT_TRUE(a == b);  // libc malloc ist global, alle Instanzen aequivalent
}

// ─────────────────────────────────────────────────────────────────────────────
// (3) Runtime allocate/deallocate
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, AllocateAndDeallocateRoundtrip) {
    axis_06::StdMalloc m{};
    void* p = m.allocate(128, 16);
    ASSERT_NE(p, nullptr);
    m.deallocate(p, 128, 16);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto stats = m.statistics();
    EXPECT_EQ(stats.allocation_count,   1u);
    EXPECT_EQ(stats.deallocation_count, 1u);
    EXPECT_EQ(stats.total_bytes_in_use, 0u);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// (4) Sub-Concepts Runtime: zero_allocate + reallocate
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, ZeroAllocateReturnsZeroedMemory) {
    axis_06::StdMalloc m{};
    void* p = m.zero_allocate(8, sizeof(int));  // 8 * 4 = 32 Byte
    ASSERT_NE(p, nullptr);
    auto const* bytes = static_cast<unsigned char*>(p);
    for (std::size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(bytes[i], 0u) << "Byte " << i << " sollte 0 sein";
    }
    std::free(p);  // calloc-allocated -> std::free OK (NICHT _aligned_free)
}

TEST(V41_TopicAllocatorAxis06, ReallocateGrowsAllocation) {
    axis_06::StdMalloc m{};
    void* p = m.allocate(16, alignof(std::max_align_t));
    ASSERT_NE(p, nullptr);
    std::memcpy(p, "12345678901234", 14);

    void* np = m.reallocate(p, 16, 64, alignof(std::max_align_t));
    ASSERT_NE(np, nullptr);
    // Inhalt sollte erhalten sein (portable alloc+memcpy+free pattern)
    EXPECT_EQ(std::memcmp(np, "12345678901234", 14), 0);

    m.deallocate(np, 64, alignof(std::max_align_t));
}

// ─────────────────────────────────────────────────────────────────────────────
// (5) CRTP-Delegate
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, CRTPDelegateAllocateDeallocate) {
    axis_06::StdMalloc m{};
    auto* base_ref = static_cast<axis_06::AllocatorStrategyBase<axis_06::StdMalloc>*>(&m);

    void* p = base_ref->allocate(256, 16);
    ASSERT_NE(p, nullptr);
    base_ref->deallocate(p, 256, 16);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto stats = base_ref->statistics();
    EXPECT_EQ(stats.allocation_count,   1u);
    EXPECT_EQ(stats.deallocation_count, 1u);
#endif
}
