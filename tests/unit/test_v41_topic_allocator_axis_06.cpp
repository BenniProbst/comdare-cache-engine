// V41.F.6.1.A Test fuer Topic Allocator Achse 6 (Concept-Beweis + Runtime)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A Pilot

#include <topics/allocator/concepts/topic_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_subaxes_aa1_to_aa7.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_strategy_base.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_std_malloc.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace topic_alloc       = comdare::cache_engine::allocator;
namespace topic_alloc_cpts  = comdare::cache_engine::allocator::concepts;
namespace axis_06           = comdare::cache_engine::allocator::axis_06_allocator;
namespace axis_06_cpts      = comdare::cache_engine::allocator::axis_06_allocator::concepts;
namespace subaxes           = comdare::cache_engine::allocator::axis_06_allocator::subaxes;

// ─────────────────────────────────────────────────────────────────────────────
// Concept-Konformanz-Beweise (Compile-Time)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesTopicConcept) {
    static_assert(topic_alloc_cpts::AllocatorComponent<axis_06::StdMalloc>,
        "StdMalloc muss Topic-Concept AllocatorComponent erfuellen");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocSatisfiesAxisConcept) {
    static_assert(axis_06_cpts::AllocatorStrategy<axis_06::StdMalloc>,
        "StdMalloc muss Achsen-Concept AllocatorStrategy erfuellen "
        "(alle 3 Pflicht-API-Gruppen)");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, AxisConceptIsStrongerThanTopicConcept) {
    // Achsen-Concept impliziert Topic-Concept (Subset-Beziehung)
    static_assert(
        axis_06_cpts::AllocatorStrategy<axis_06::StdMalloc>
            ? topic_alloc_cpts::AllocatorComponent<axis_06::StdMalloc>
            : true,
        "Achsen-Concept muss Topic-Concept als Vorbedingung haben");
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Negative-Beweis: dummy ohne Concept-Konformanz wird abgelehnt
// ─────────────────────────────────────────────────────────────────────────────

struct DummyNotAllocator {
    // weder topic_tag noch raw_allocate
};

TEST(V41_TopicAllocatorAxis06, DummyDoesNotSatisfyTopicConcept) {
    static_assert(!topic_alloc_cpts::AllocatorComponent<DummyNotAllocator>,
        "Dummy ohne topic_tag darf Topic-Concept NICHT erfuellen");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, DummyDoesNotSatisfyAxisConcept) {
    static_assert(!axis_06_cpts::AllocatorStrategy<DummyNotAllocator>,
        "Dummy ohne Pflicht-API darf Achsen-Concept NICHT erfuellen");
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Compile-Time-Eigenschaften (Pflicht-API-Gruppe 2)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, StdMallocCompileTimeProperties) {
    using SM = axis_06::StdMalloc;
    static_assert(std::same_as<SM::topic_tag, topic_alloc_cpts::AllocatorTopicTag>);
    static_assert(std::same_as<SM::axis_tag,  subaxes::size_class_schema_tag>);
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

// ─────────────────────────────────────────────────────────────────────────────
// Runtime-Allocation-API (Pflicht-API-Gruppe 1)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, AllocateAndDeallocateRoundtrip) {
    axis_06::StdMalloc m{};
    auto stats0 = m.statistics();
    EXPECT_EQ(stats0.allocation_count,   0u);
    EXPECT_EQ(stats0.deallocation_count, 0u);

    void* p = m.raw_allocate(128, 16);
    ASSERT_NE(p, nullptr);

    auto stats1 = m.statistics();
    EXPECT_EQ(stats1.allocation_count, 1u);
    EXPECT_GE(stats1.total_bytes_in_use, 128u);

    m.raw_deallocate(p, 128, 16);
    auto stats2 = m.statistics();
    EXPECT_EQ(stats2.deallocation_count, 1u);
    EXPECT_EQ(stats2.total_bytes_in_use, 0u);
}

TEST(V41_TopicAllocatorAxis06, ResetClearsStatistics) {
    axis_06::StdMalloc m{};
    void* p = m.raw_allocate(64, 8);
    ASSERT_NE(p, nullptr);
    m.raw_deallocate(p, 64, 8);

    m.reset();
    auto stats = m.statistics();
    EXPECT_EQ(stats.allocation_count,   0u);
    EXPECT_EQ(stats.deallocation_count, 0u);
    EXPECT_EQ(stats.total_bytes_in_use, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CRTP-Basis: Default-Methoden funktionieren (allocate/deallocate/statistics/reset)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, CRTPDelegateAllocateDeallocate) {
    axis_06::StdMalloc m{};
    auto* base_ref = static_cast<axis_06::AllocatorStrategyBase<axis_06::StdMalloc>*>(&m);

    void* p = base_ref->allocate(256, 16);
    ASSERT_NE(p, nullptr);

    auto stats = base_ref->statistics();
    EXPECT_EQ(stats.allocation_count, 1u);

    base_ref->deallocate(p, 256, 16);
    base_ref->reset();
    auto stats_after_reset = base_ref->statistics();
    EXPECT_EQ(stats_after_reset.allocation_count, 0u);
}
