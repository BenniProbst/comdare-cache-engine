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
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_snmalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_pmr_resource.hpp>

// V41.F.6.1.C Stufe 1: Registry-Smoke-Test (W6 zentralisierte Topic-Registrierung)
#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_registry.hpp>

// V41.F.6.1.D Stufe 4: TopicConfigSet + PermutationEngine
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

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

// ─────────────────────────────────────────────────────────────────────────────
// (6) F.6.1.C Batch 1: Mimalloc-Wrapper (A04 Free-List-Sharding)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, MimallocConceptConformance) {
    static_assert(topic_alloc_cpts::AllocatorComponent<axis_06::MimallocAllocator>);
    static_assert(axis_06_cpts::AllocatorStrategy<axis_06::MimallocAllocator>);
    static_assert(axis_06_cpts::CacheEnginePermutationStrategy<axis_06::MimallocAllocator>);
    static_assert(axis_06_cpts::ZeroingStrategy<axis_06::MimallocAllocator>);
    static_assert(axis_06_cpts::ReallocatingStrategy<axis_06::MimallocAllocator>);
    static_assert(axis_06_cpts::IntrospectableStrategy<axis_06::MimallocAllocator>);
    static_assert(axis_06_cpts::ReclaimableStrategy<axis_06::MimallocAllocator>);
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, MimallocCompileTimeProperties) {
    using M = axis_06::MimallocAllocator;
    static_assert(std::same_as<M::axis_tag, subaxes::freelist_topology_tag>);
    static_assert(M::family_id::value == 4);
    static_assert(M::is_thread_safe());
    static_assert(M::supports_pmr());
    EXPECT_FALSE(M::name().empty());
    EXPECT_FALSE(M::family_name().empty());
}

TEST(V41_TopicAllocatorAxis06, MimallocAllocateRoundtrip) {
    axis_06::MimallocAllocator m{};
    void* p = m.allocate(128, 16);
    ASSERT_NE(p, nullptr);
    m.deallocate(p, 128, 16);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    EXPECT_EQ(m.statistics().allocation_count, 1u);
    EXPECT_EQ(m.statistics().deallocation_count, 1u);
#endif
}

TEST(V41_TopicAllocatorAxis06, MimallocCollectIsCallable) {
    axis_06::MimallocAllocator m{};
    void* p = m.allocate(64, 8);
    m.deallocate(p, 64, 8);
    m.collect(true);   // Reclaim-API darf nicht crashen
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (7) F.6.1.C Batch 1: Snmalloc-Wrapper (A07 Message-Passing)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, SnmallocConceptConformance) {
    static_assert(topic_alloc_cpts::AllocatorComponent<axis_06::SnmallocAllocator>);
    static_assert(axis_06_cpts::AllocatorStrategy<axis_06::SnmallocAllocator>);
    static_assert(axis_06_cpts::CacheEnginePermutationStrategy<axis_06::SnmallocAllocator>);
    static_assert(axis_06_cpts::ZeroingStrategy<axis_06::SnmallocAllocator>);
    static_assert(axis_06_cpts::ReallocatingStrategy<axis_06::SnmallocAllocator>);
    static_assert(axis_06_cpts::IntrospectableStrategy<axis_06::SnmallocAllocator>);
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, SnmallocCompileTimeProperties) {
    using S = axis_06::SnmallocAllocator;
    static_assert(std::same_as<S::axis_tag, subaxes::thread_locality_tag>);
    static_assert(S::family_id::value == 7);
    static_assert(S::is_thread_safe());
}

TEST(V41_TopicAllocatorAxis06, SnmallocAllocateRoundtrip) {
    axis_06::SnmallocAllocator s{};
    void* p = s.allocate(256, 32);
    ASSERT_NE(p, nullptr);
    s.deallocate(p, 256, 32);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    EXPECT_EQ(s.statistics().allocation_count, 1u);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// (8) F.6.1.C Batch 1: PMR-Resource-Wrapper (A22 Halpern N3916)
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, PmrResourceConceptConformance) {
    static_assert(topic_alloc_cpts::AllocatorComponent<axis_06::PmrResourceAllocator>);
    static_assert(axis_06_cpts::AllocatorStrategy<axis_06::PmrResourceAllocator>);
    static_assert(axis_06_cpts::CacheEnginePermutationStrategy<axis_06::PmrResourceAllocator>);
    // KEIN ZeroingStrategy/ReallocatingStrategy fuer PMR
    static_assert(!axis_06_cpts::ZeroingStrategy<axis_06::PmrResourceAllocator>);
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, PmrResourceCompileTimeProperties) {
    using P = axis_06::PmrResourceAllocator;
    static_assert(std::same_as<P::axis_tag, subaxes::allocation_policy_tag>);
    static_assert(P::family_id::value == 22);
    static_assert(P::supports_pmr());
    EXPECT_EQ(std::string_view{P::name()}, "pmr_resource");
}

TEST(V41_TopicAllocatorAxis06, PmrResourceAllocateRoundtrip) {
    axis_06::PmrResourceAllocator p{};   // default = std::pmr::new_delete_resource()
    void* mem = p.allocate(64, 8);
    ASSERT_NE(mem, nullptr);
    p.deallocate(mem, 64, 8);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    EXPECT_EQ(p.statistics().allocation_count, 1u);
#endif
}

TEST(V41_TopicAllocatorAxis06, PmrResourceWithCustomResource) {
    // Test mit explicit monotonic_buffer_resource (kein delete bis Reset)
    std::byte buffer[1024];
    std::pmr::monotonic_buffer_resource mono(buffer, sizeof(buffer));
    axis_06::PmrResourceAllocator p{&mono};
    EXPECT_EQ(p.underlying_resource(), &mono);

    void* mem = p.allocate(128, 16);
    ASSERT_NE(mem, nullptr);
    // monotonic_buffer_resource: dealloc ist no-op, kein crash
    p.deallocate(mem, 128, 16);
}

// ─────────────────────────────────────────────────────────────────────────────
// (9) F.6.1.C Stufe 1: Registry-Smoke-Test (W6 zentralisierte Topic-Registrierung)
// ─────────────────────────────────────────────────────────────────────────────

namespace flags = comdare::cache_engine::allocator::axis_06_allocator::flags;

TEST(V41_TopicAllocatorAxis06, FlagsHeaderIsTypedConstexpr) {
    // axis_06_allocator_flags.hpp via configure_file generiert
    static_assert(std::is_same_v<decltype(flags::std_enabled),       const bool>);
    static_assert(std::is_same_v<decltype(flags::mimalloc_enabled),  const bool>);
    static_assert(std::is_same_v<decltype(flags::snmalloc_enabled),  const bool>);
    static_assert(std::is_same_v<decltype(flags::pmr_enabled),       const bool>);
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, RegistryAllVendorsCount) {
    using AllV = axis_06::AllVendors;
    static_assert(boost::mp11::mp_size<AllV>::value == 4,
        "Stufe 1: 4 Vendor in AllVendors (Std + Mi + Sn + PMR). Batch 2-8 ergaenzt spaeter.");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, RegistryEnabledVendorsNonEmpty) {
    using EnabledV = axis_06::EnabledVendors;
    // Stufe 1: is_enabled<T> = mp_bool<true> -> EnabledVendors = AllVendors
    static_assert(boost::mp11::mp_size<EnabledV>::value > 0,
        "Mindestens 1 Vendor muss enabled sein");
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, RegistryMpForEachIteration) {
    // Compile-Time Iteration ueber EnabledVendors via mp_for_each
    // Anzahl ist dynamisch (haengt von CMake-Flags COMDARE_AXIS_06_USE_<VENDOR> ab):
    //   build-pilot ohne -DCOMDARE_BUILD_PERMUTATIONS=ON: USE_STD=1, MIMALLOC=0, SNMALLOC=0, PMR=1 -> 2 enabled
    //   build mit Vendor-Build: alle 4 USE=1 -> 4 enabled
    int counted = 0;
    boost::mp11::mp_for_each<axis_06::EnabledVendors>([&counted]<class V>(V) {
        // pro Vendor: pruefen dass es das AllocatorStrategy-Concept erfuellt
        static_assert(axis_06_cpts::AllocatorStrategy<V>,
            "Jeder Vendor in EnabledVendors muss AllocatorStrategy erfuellen");
        ++counted;
    });
    constexpr auto expected = boost::mp11::mp_size<axis_06::EnabledVendors>::value;
    EXPECT_EQ(static_cast<std::size_t>(counted), expected);
    EXPECT_GE(expected, 1u);  // mindestens STD/PMR (immer verfuegbar)
}

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1.I Stufe 3 LIVE: Observer-Notify-Pattern (Pflicht wenn STATISTICS=ON)
// ───────────────────────────────────────────────────────────────────────────
#ifdef COMDARE_CE_ENABLE_STATISTICS

// (a) MeasurableComponent Concept erfuellt — observer_t als MeasurableObserver<snapshot_t>
TEST(V41_TopicAllocatorAxis06_Stufe3, WrapperHasObserverAlias) {
    using ObserverT_Std = axis_06::StdMalloc::observer_t;
    using SnapshotT_Std = axis_06::StdMalloc::snapshot_t;
    static_assert(std::is_same_v<ObserverT_Std,
                                 ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Std>>,
                  "StdMalloc::observer_t muss MeasurableObserver<snapshot_t> sein");

    using ObserverT_Mi  = axis_06::MimallocAllocator::observer_t;
    using SnapshotT_Mi  = axis_06::MimallocAllocator::snapshot_t;
    static_assert(std::is_same_v<ObserverT_Mi,
                                 ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Mi>>,
                  "MimallocAllocator::observer_t muss MeasurableObserver<snapshot_t> sein");

    using ObserverT_Sn  = axis_06::SnmallocAllocator::observer_t;
    using SnapshotT_Sn  = axis_06::SnmallocAllocator::snapshot_t;
    static_assert(std::is_same_v<ObserverT_Sn,
                                 ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Sn>>,
                  "SnmallocAllocator::observer_t muss MeasurableObserver<snapshot_t> sein");

    using ObserverT_Pmr = axis_06::PmrResourceAllocator::observer_t;
    using SnapshotT_Pmr = axis_06::PmrResourceAllocator::snapshot_t;
    static_assert(std::is_same_v<ObserverT_Pmr,
                                 ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Pmr>>,
                  "PmrResourceAllocator::observer_t muss MeasurableObserver<snapshot_t> sein");
    SUCCEED();
}

// (b) Observer-Callback wird bei allocate() benachrichtigt
TEST(V41_TopicAllocatorAxis06_Stufe3, ObserverNotifiesOnAllocate_StdMalloc) {
    axis_06::StdMalloc m{};
    int events = 0;
    std::uint64_t last_count = 0;
    m.observer().on_event([&events, &last_count](auto const& snap) {
        ++events;
        last_count = snap.allocation_count;
    });
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(events, 1);
    EXPECT_EQ(last_count, 1u);

    m.deallocate(p, 64, 8);
    EXPECT_EQ(events, 2);
}

// (c) Observer-Notify auch bei zero_allocate (separat) + reallocate (separat)
TEST(V41_TopicAllocatorAxis06_Stufe3, ObserverNotifiesOnZeroAllocAndRealloc_StdMalloc) {
    axis_06::StdMalloc m{};
    int events = 0;
    m.observer().on_event([&events](auto const&) { ++events; });

    // zero_allocate -> std::free Pfad (separater Lifecycle)
    void* zp = m.zero_allocate(4, 16);
    ASSERT_NE(zp, nullptr);
    EXPECT_EQ(events, 1);
    std::free(zp);  // zero_allocate verwendet std::calloc — passendes std::free, KEIN reallocate

    // reallocate ueber portable_aligned_alloc Lifecycle (separat)
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(events, 2);
    void* np = m.reallocate(p, 64, 128, 8);
    ASSERT_NE(np, nullptr);
    EXPECT_GE(events, 3);  // reallocate macht 1 notify im Erfolgs-Fall

    m.deallocate(np, 128, 8);
    EXPECT_GE(events, 4);
}

// (d) reset() benachrichtigt Observer mit leeren Stats
TEST(V41_TopicAllocatorAxis06_Stufe3, ResetNotifiesObserverWithClearedStats) {
    axis_06::StdMalloc m{};
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);

    std::uint64_t last_alloc_count = 0;
    m.observer().on_event([&last_alloc_count](auto const& snap) {
        last_alloc_count = snap.allocation_count;
    });
    m.reset();
    EXPECT_EQ(last_alloc_count, 0u);

    m.deallocate(p, 64, 8);
}

// (e) Observer ohne registriertes Callback -> notify ist no-op (kein Crash)
TEST(V41_TopicAllocatorAxis06_Stufe3, ObserverWithoutCallbackIsNoOp) {
    axis_06::StdMalloc m{};
    EXPECT_FALSE(m.observer().has_callback());
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);  // kein Crash trotz fehlendem Callback
    m.deallocate(p, 64, 8);
    SUCCEED();
}

// (f) Mimalloc + Snmalloc + PMR: Observer-Notify funktioniert genauso
TEST(V41_TopicAllocatorAxis06_Stufe3, ObserverNotifiesOnAllocate_OtherVendors) {
    {
        axis_06::MimallocAllocator m{};
        int events = 0;
        m.observer().on_event([&events](auto const&) { ++events; });
        void* p = m.allocate(64, 8);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(events, 1);
        m.deallocate(p, 64, 8);
    }
    {
        axis_06::SnmallocAllocator m{};
        int events = 0;
        m.observer().on_event([&events](auto const&) { ++events; });
        void* p = m.allocate(64, 8);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(events, 1);
        m.deallocate(p, 64, 8);
    }
    {
        axis_06::PmrResourceAllocator m{};
        int events = 0;
        m.observer().on_event([&events](auto const&) { ++events; });
        void* p = m.allocate(64, 8);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(events, 1);
        m.deallocate(p, 64, 8);
    }
}

#endif  // COMDARE_CE_ENABLE_STATISTICS

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1.D Stufe 4: PermutationEngine + TopicConfigSet (Doku §15.4 / §15.7)
// ───────────────────────────────────────────────────────────────────────────

namespace perms = comdare::cache_engine::permutations;
namespace alloc = comdare::cache_engine::allocator;

// (a) TopicConfigSet hat StaticAxisVariants = EnabledVendors
TEST(V41_TopicAllocatorAxis06_Stufe4, TopicConfigSetHasEnabledVendors) {
    using TCS = alloc::TopicConfigSet;
    static_assert(std::is_same_v<typename TCS::StaticAxisVariants,
                                  alloc::axis_06_allocator::EnabledVendors>,
        "TopicConfigSet::StaticAxisVariants muss EnabledVendors sein");
    SUCCEED();
}

// (b) PermutationEngine kompiliert mit EINEM TopicConfigSet
TEST(V41_TopicAllocatorAxis06_Stufe4, PermutationEngineSingleTopic) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    static_assert(Engine::arity == 1, "1 Topic-Achse");
    constexpr auto cnt = Engine::count();
    // count = Produkt aller EnabledVendors-Sizes. Build-pilot: std + pmr = 2 enabled.
    EXPECT_GE(cnt, 1u);
    EXPECT_EQ(cnt, boost::mp11::mp_size<alloc::axis_06_allocator::EnabledVendors>::value);
}

// (c) for_each_permutation iteriert genau count() Permutationen
TEST(V41_TopicAllocatorAxis06_Stufe4, ForEachPermutationCount) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    int seen = 0;
    Engine::for_each_permutation([&seen]<class P>(){
        ++seen;
        // Jede Permutation hat hash() != 0 (FNV-1a startet mit Offset-Basis)
        constexpr auto h = P::hash();
        static_assert(h != 0, "FNV-1a Hash darf nicht 0 sein");
        (void)h;
    });
    EXPECT_EQ(static_cast<std::size_t>(seen), Engine::count());
}

// (d) PermTuple<V>-Hash ist stabil + verschieden pro Vendor
TEST(V41_TopicAllocatorAxis06_Stufe4, PermTupleHashIsStableAndDistinct) {
    using P_Std  = perms::PermTuple<axis_06::StdMalloc>;
    using P_Mi   = perms::PermTuple<axis_06::MimallocAllocator>;
    using P_Pmr  = perms::PermTuple<axis_06::PmrResourceAllocator>;

    constexpr auto h_std = P_Std::hash();
    constexpr auto h_mi  = P_Mi::hash();
    constexpr auto h_pmr = P_Pmr::hash();

    EXPECT_NE(h_std, h_mi);
    EXPECT_NE(h_std, h_pmr);
    EXPECT_NE(h_mi,  h_pmr);

    // Stabilitaet: 2x Aufruf liefert gleichen Hash
    EXPECT_EQ(h_std, P_Std::hash());
}

// (e) F.6.1.H Pflicht-Constraint: mp_all_of has_non_empty_axis
TEST(V41_TopicAllocatorAxis06_Stufe4, MinOneVendorPerAxisConstraintLive) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    static_assert(Engine::non_empty_axis_count == Engine::arity,
        "alle Achsen muessen min. 1 Vendor haben (sonst greift Static-Assert)");
    EXPECT_EQ(Engine::non_empty_axis_count, Engine::arity);
}

// (f) for_each_filtered mit AlwaysTrue == identisch mit for_each_permutation
TEST(V41_TopicAllocatorAxis06_Stufe4, FilterAlwaysTrueIsAllPermutations) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    int filtered_count = 0;
    Engine::for_each_filtered<perms::AlwaysTrue>([&filtered_count]<class>(){
        ++filtered_count;
    });
    EXPECT_EQ(static_cast<std::size_t>(filtered_count), Engine::count());
}

// (g) for_each_filtered mit AlwaysFalse = 0 Aufrufe
TEST(V41_TopicAllocatorAxis06_Stufe4, FilterAlwaysFalseIsZero) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    int filtered_count = 0;
    Engine::for_each_filtered<perms::AlwaysFalse>([&filtered_count]<class>(){
        ++filtered_count;
    });
    EXPECT_EQ(filtered_count, 0);
    static_assert(Engine::count_filtered<perms::AlwaysFalse>() == 0,
        "Diagnose-Counter mit AlwaysFalse muss 0 sein");
}

// (h) AxisFullJoin (Stufe 3 Skelett): mp_append + mp_unique
TEST(V41_TopicAllocatorAxis06_Stufe4, AxisFullJoinDeduplicatesVariants) {
    namespace mp = boost::mp11;
    // Beispiel: cache-engine Defaults + 1 Pruefling der StdMalloc auch nutzt
    using DefaultList   = mp::mp_list<axis_06::StdMalloc, axis_06::MimallocAllocator>;
    using PrueflingList = mp::mp_list<axis_06::StdMalloc, axis_06::SnmallocAllocator>;  // StdMalloc doppelt
    using Joined = perms::AxisFullJoin<DefaultList, PrueflingList>;
    // mp_unique entfernt das doppelte StdMalloc -> 3 statt 4
    static_assert(mp::mp_size<Joined>::value == 3,
        "AxisFullJoin muss Duplikate via mp_unique entfernen");
    SUCCEED();
}
