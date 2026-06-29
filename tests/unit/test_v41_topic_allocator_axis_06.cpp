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
#include <topics/allocator/axis_06_allocator/axis_06_allocator_pool_resource.hpp> // R7.4 resource_ownership Owned-Anker
// V41.F.6.1 Batch 2 Vendor (2026-05-26)
#include <topics/allocator/axis_06_allocator/axis_06_allocator_jemalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_tcmalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_dlmalloc.hpp>

// V41.F.6.1.C Stufe 1: Registry-Smoke-Test (W6 zentralisierte Topic-Registrierung)
#include <axes/alloc/axis_06_allocator_flags.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_registry.hpp>

// V41.F.6.1.D Stufe 4: TopicConfigSet + PermutationEngine
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <permutations/permutation_engine.hpp>

// V41.F.6.1.G CacheEngineBuilder CLI-Flag-Builder
#include <permutations/permutation_build_command.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <numeric>
#include <memory>
#include <memory_resource>

namespace topic_alloc      = comdare::cache_engine::allocator;
namespace topic_alloc_cpts = comdare::cache_engine::allocator::concepts;
namespace axis_06          = comdare::cache_engine::allocator::axis_06_allocator;
namespace axis_06_cpts     = comdare::cache_engine::allocator::axis_06_allocator::concepts;
namespace subaxes          = comdare::cache_engine::allocator::axis_06_allocator::subaxes;

// ─────────────────────────────────────────────────────────────────────────────
// V41.F.6.1 Batch 3 (User-Direktive 2026-05-26): TYPED_TEST_SUITE
// statt wiederholter Einzeltests pro Vendor. mp_apply konvertiert die zentrale
// AllVendors mp_list automatisch in ::testing::Types — KEIN Sync-Aufwand bei
// Erweiterung der Vendor-Liste. Test-Granularitaet bleibt pro Vendor erhalten
// (GTest expandiert TYPED_TEST in 1 Test pro Type).
// ─────────────────────────────────────────────────────────────────────────────

// GTest TYPED_TEST_SUITE Boilerplate im globalen Scope (Macro vertraegt
// keine namespace-Qualifier, deshalb hier direkt, nicht in test_helpers::).
template <class... Vs>
using ToGTestTypes = ::testing::Types<Vs...>;

/// MP11 -> GTest-Typliste: synchron mit axis_06::AllVendors per mp_apply
/// Erweiterung der AllVendors-mp_list = automatische Test-Erweiterung
using AllVendorTypes = boost::mp11::mp_apply<ToGTestTypes, axis_06::AllVendors>;

template <class T>
class AllocatorVendorTest : public ::testing::Test {};

TYPED_TEST_SUITE(AllocatorVendorTest, AllVendorTypes);

// ─────────────────────────────────────────────────────────────────────────────
// V41.F.6.1 (User-Direktive 2026-05-26): Runtime-Permutations-Liste
// Pro TYPED_TEST iterieren wir zusaetzlich ueber eine constexpr-Konfigurations-
// Liste (size/alignment-Kombinationen), damit die dynamische Seite jeder Vendor-
// Variante mitgetestet wird — nicht nur 1 fixe Konfiguration pro Vendor.
// ─────────────────────────────────────────────────────────────────────────────
struct AllocConfig {
    std::size_t bytes;
    std::size_t alignment;
};
constexpr std::array<AllocConfig, 7> kTestAllocConfigs{{
    {8, 8},       // word-aligned tiny
    {64, 8},      // cache-line-aligned small
    {128, 16},    // SSE-aligned
    {256, 32},    // AVX-aligned
    {1024, 64},   // cache-line-aligned medium
    {4096, 4096}, // page-aligned + page-sized
    {16384, 16},  // larger block, default alignment
}};

constexpr std::array<std::pair<std::size_t, std::size_t>, 4> kTestZeroAllocConfigs{{
    {1, 64},    // 1 element x 64 bytes
    {4, 16},    // 4 element x 16 bytes
    {16, 64},   // 16 element x 64 bytes (1 KB)
    {128, 128}, // 128 element x 128 bytes (16 KB)
}};

// Pflicht-Concepts werden pro Vendor compile-time geprueft (1 Test je Vendor)
TYPED_TEST(AllocatorVendorTest, ConceptConformance) {
    static_assert(axis_06_cpts::AllocatorStrategy<TypeParam>, "Pflicht: AllocatorStrategy (PMR-Standard)");
    static_assert(
        axis_06_cpts::CacheEnginePermutationStrategy<TypeParam>,
        "Pflicht: CacheEnginePermutationStrategy (cache-engine-spec, mit statistics+observer wenn STATISTICS=ON)");
    SUCCEED();
}

// Identifikation: name + flag_suffix + family_id != 0
TYPED_TEST(AllocatorVendorTest, Identification) {
    static_assert(!TypeParam::name().empty(), "name() darf nicht leer sein");
    static_assert(!TypeParam::family_name().empty(), "family_name() darf nicht leer sein");
    static_assert(!TypeParam::flag_suffix().empty(), "flag_suffix() darf nicht leer sein (F.6.1.G CLI)");
    static_assert(TypeParam::family_id::value > 0, "family_id muss > 0 (A01-A23 mapping)");
    SUCCEED();
}

// V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
// Pro Vendor MUSS jede Property eine Antwort liefern (Wert variabel, aber Methode Pflicht).
TYPED_TEST(AllocatorVendorTest, SonderfallPropertiesQueryable) {
    using PG = axis_06_cpts::ProgressGuarantee;
    // Properties existieren (Compile-Pflicht via CacheEnginePermutationStrategy)
    [[maybe_unused]] constexpr bool a  = TypeParam::has_native_aligned_alloc();
    [[maybe_unused]] constexpr bool b  = TypeParam::requires_explicit_init();
    [[maybe_unused]] constexpr bool c  = TypeParam::supports_numa_node_hint();
    [[maybe_unused]] constexpr bool e  = TypeParam::supports_thread_local_cache();
    [[maybe_unused]] constexpr bool f  = TypeParam::requires_specialized_hardware();
    [[maybe_unused]] constexpr PG   pg = TypeParam::progress_guarantee(); // Batch 7 Stufen-Refactor
    [[maybe_unused]] constexpr axis_06_cpts::ResourceOwnership ro =
        TypeParam::resource_ownership(); // R7.4 Pflicht-Property
    // Konsistenz-Checks: Sonderfaelle pro Batch
    if constexpr (std::is_same_v<TypeParam, axis_06::ScallocAllocator>) {
        static_assert(!a, "Scalloc-Sonderfall: keine native aligned_alloc API");
        static_assert(pg == PG::LockFree, "Scalloc Spans-Free-List ist lock-free");
    }
    if constexpr (std::is_same_v<TypeParam, axis_06::RPMallocAllocator>) {
        static_assert(b, "RPMalloc-Sonderfall: requires_explicit_init=true");
    }
    if constexpr (std::is_same_v<TypeParam, axis_06::NUMAllocAllocator>) {
        static_assert(c, "NUMAlloc-Sonderfall: supports_numa_node_hint=true");
    }
    if constexpr (std::is_same_v<TypeParam, axis_06::MichaelLockFreeAllocator>) {
        static_assert(pg == PG::LockFree, "MichaelLockFree: progress_guarantee=LockFree");
    }
    if constexpr (std::is_same_v<TypeParam, axis_06::PIMMallocAllocator>) {
        static_assert(f, "PIM-Malloc-Sonderfall (Batch 6): requires_specialized_hardware=true");
    }
    if constexpr (std::is_same_v<TypeParam, axis_06::CrystallineAllocator>) {
        static_assert(pg == PG::WaitFree, "Crystalline-Sonderfall (Batch 7): progress_guarantee=WaitFree");
        static_assert(pg >= PG::LockFree, "WaitFree impliziert LockFree (Stufen-Ordnung)");
    }
    SUCCEED();
}

// R7.4: resource_ownership() grenzt die PMR-Familie (A22) TYPSICHER ab — POOL besitzt seine
// memory_resource selbst (Owned), PMR reicht eine externe durch (Borrowed), die gesamte malloc-
// Familie verwaltet keine pmr-Resource (None, via AllocatorStrategyBase-Default). Orthogonal zu
// supports_pmr() (das ist fuer alle drei true). Reiner Compile-Time-Beweis.
TEST(V41_TopicAllocatorAxis06, ResourceOwnershipDistinguishesPoolFromPmr) {
    using RO = axis_06_cpts::ResourceOwnership;
    // Die eigentliche Abgrenzung: POOL=Owned vs PMR=Borrowed.
    static_assert(axis_06::PoolResourceAllocator::resource_ownership() == RO::Owned,
                  "POOL besitzt eine eigene unsynchronized_pool_resource (Owned)");
    static_assert(axis_06::PmrResourceAllocator::resource_ownership() == RO::Borrowed,
                  "PMR reicht eine extern besessene memory_resource durch (Borrowed)");
    static_assert(RO::Owned != RO::Borrowed, "Owned und Borrowed sind verschieden");
    // malloc-Familie erbt den None-Default aus AllocatorStrategyBase (kein 25x-Hardcode):
    static_assert(axis_06::StdMalloc::resource_ownership() == RO::None, "StdMalloc: None (Default)");
    static_assert(axis_06::JemallocAllocator::resource_ownership() == RO::None, "Jemalloc: None (Default)");
    static_assert(axis_06::MimallocAllocator::resource_ownership() == RO::None, "Mimalloc: None (Default)");
    static_assert(RO::None != RO::Owned && RO::None != RO::Borrowed, "None ist von beiden verschieden");
    // Orthogonalitaet zu supports_pmr(): POOL, PMR UND jemalloc liefern supports_pmr()==true,
    // aber NUR POOL/PMR haben ein eigenes/geborgtes Resource-Objekt -> resource_ownership trennt feiner.
    static_assert(axis_06::JemallocAllocator::supports_pmr() &&
                      axis_06::JemallocAllocator::resource_ownership() == RO::None,
                  "supports_pmr() != resource_ownership(): jemalloc ist pmr-nutzbar, besitzt aber keine Resource");
    SUCCEED();
}

// Roundtrip: allocate + deallocate ueber Runtime-Konfigurations-Liste
// (User-Direktive 2026-05-26: dynamische Seite testen — for-loop ueber constexpr Liste)
TYPED_TEST(AllocatorVendorTest, AllocateDeallocateRoundtripAllConfigs) {
    TypeParam m{};
    for (auto const& cfg : kTestAllocConfigs) {
        void* p = m.allocate(cfg.bytes, cfg.alignment);
        ASSERT_NE(p, nullptr) << "Vendor " << TypeParam::name() << " allocate(" << cfg.bytes << ", " << cfg.alignment
                              << ") failed";
        m.deallocate(p, cfg.bytes, cfg.alignment);
    }
}

// zero_allocate ueber Runtime-Konfigurations-Liste (n x size Permutationen)
// if constexpr Guard: nur fuer Vendor die ZeroingStrategy Sub-Concept erfuellen
// (PmrResourceAllocator z.B. erfuellt es nicht — PMR-Interface bietet das nicht).
TYPED_TEST(AllocatorVendorTest, ZeroAllocateRoundtripAllConfigs) {
    if constexpr (axis_06_cpts::ZeroingStrategy<TypeParam>) {
        TypeParam m{};
        for (auto const& [n, size] : kTestZeroAllocConfigs) {
            void* p = m.zero_allocate(n, size);
            ASSERT_NE(p, nullptr) << "Vendor " << TypeParam::name() << " zero_allocate(" << n << ", " << size
                                  << ") failed";
            // Pruefe dass Memory zero-initialisiert ist (calloc-Pflicht)
            auto* bytes = static_cast<unsigned char*>(p);
            for (std::size_t i = 0; i < n * size; ++i) {
                ASSERT_EQ(bytes[i], 0u) << "zero_allocate Byte " << i << " nicht 0 (Vendor " << TypeParam::name()
                                        << ")";
            }
            // zero_allocate verwendet std::calloc Pfad — std::free statt deallocate
            // (Heap-Crossover-Vermeidung wenn HAVE=OFF Fallback)
            std::free(p);
        }
    }
    SUCCEED(); // Vendor ohne ZeroingStrategy: kein Body, kein Failure
}

// reallocate ueber Runtime-Konfigurations-Liste (alloc -> realloc -> dealloc)
// Sub-Concept ReallocatingStrategy ggf. nicht erfuellt -> if constexpr Guard.
TYPED_TEST(AllocatorVendorTest, ReallocateRoundtripAllConfigs) {
    if constexpr (axis_06_cpts::ReallocatingStrategy<TypeParam>) {
        TypeParam m{};
        for (auto const& cfg : kTestAllocConfigs) {
            std::size_t new_bytes = cfg.bytes * 2;
            void*       p         = m.allocate(cfg.bytes, cfg.alignment);
            ASSERT_NE(p, nullptr);
            void* np = m.reallocate(p, cfg.bytes, new_bytes, cfg.alignment);
            ASSERT_NE(np, nullptr) << "Vendor " << TypeParam::name() << " reallocate(" << cfg.bytes << "->" << new_bytes
                                   << ") failed";
            m.deallocate(np, new_bytes, cfg.alignment);
        }
    }
    SUCCEED();
}

// Stufe 3 Observer-Notify-Pflicht ueber Runtime-Konfigurations-Liste
#ifdef COMDARE_CE_ENABLE_STATISTICS
TYPED_TEST(AllocatorVendorTest, ObserverNotifyOnAllocateAndDeallocateAllConfigs) {
    TypeParam m{};
    int       events = 0;
    m.observer().on_event([&events](auto const&) { ++events; });
    int expected_events = 0;
    for (auto const& cfg : kTestAllocConfigs) {
        void* p = m.allocate(cfg.bytes, cfg.alignment);
        ASSERT_NE(p, nullptr);
        ++expected_events;
        EXPECT_EQ(events, expected_events) << "Nach allocate(" << cfg.bytes << "," << cfg.alignment << ") fehlt notify";
        m.deallocate(p, cfg.bytes, cfg.alignment);
        ++expected_events;
        EXPECT_EQ(events, expected_events)
            << "Nach deallocate(" << cfg.bytes << "," << cfg.alignment << ") fehlt notify";
    }
    // Pro AllocConfig 2 Events (alloc + dealloc)
    EXPECT_EQ(events, static_cast<int>(kTestAllocConfigs.size()) * 2);
}

// observer_t Pflicht-Alias = MeasurableObserver<snapshot_t>
TYPED_TEST(AllocatorVendorTest, ObserverAliasIsMeasurableObserverOfSnapshot) {
    using S = typename TypeParam::snapshot_t;
    using O = typename TypeParam::observer_t;
    static_assert(std::is_same_v<O, ::comdare::cache_engine::measurement::MeasurableObserver<S>>,
                  "observer_t Pflicht-Alias = MeasurableObserver<snapshot_t>");
    SUCCEED();
}
#endif

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
    void*              p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);
    m.deallocate(p, 64, 8);
    EXPECT_EQ(m.statistics().allocation_count, 1u);

    m.reset(); // = Statistik-Reset
    auto stats = m.statistics();
    EXPECT_EQ(stats.allocation_count, 0u);
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

struct DummyNotAllocator {}; // weder topic_tag noch allocate

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
    static_assert(std::same_as<SM::topic_tag, topic_alloc_cpts::AllocatorTopicTag>);
    static_assert(std::same_as<SM::axis_tag, subaxes::size_class_schema_tag>);
    static_assert(std::same_as<SM::value_type, std::byte>);
    static_assert(std::same_as<SM::size_type, std::size_t>);
    static_assert(SM::family_id::value == 22);
    static_assert(SM::is_thread_safe());
    static_assert(SM::supports_pmr());
    static_assert(SM::max_alignment() >= alignof(std::max_align_t));
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, StdMallocIdentification) {
    using SM = axis_06::StdMalloc;
    EXPECT_EQ(SM::name(), "std_malloc");
    EXPECT_FALSE(SM::family_name().empty());
}

TEST(V41_TopicAllocatorAxis06, StdMallocEquality) {
    axis_06::StdMalloc a{};
    axis_06::StdMalloc b{};
    EXPECT_TRUE(a == b); // libc malloc ist global, alle Instanzen aequivalent
}

// ─────────────────────────────────────────────────────────────────────────────
// (3) Runtime allocate/deallocate
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, AllocateAndDeallocateRoundtrip) {
    axis_06::StdMalloc m{};
    void*              p = m.allocate(128, 16);
    ASSERT_NE(p, nullptr);
    m.deallocate(p, 128, 16);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto stats = m.statistics();
    EXPECT_EQ(stats.allocation_count, 1u);
    EXPECT_EQ(stats.deallocation_count, 1u);
    EXPECT_EQ(stats.total_bytes_in_use, 0u);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// (4) Sub-Concepts Runtime: zero_allocate + reallocate
// ─────────────────────────────────────────────────────────────────────────────

TEST(V41_TopicAllocatorAxis06, ZeroAllocateReturnsZeroedMemory) {
    axis_06::StdMalloc m{};
    void*              p = m.zero_allocate(8, sizeof(int)); // 8 * 4 = 32 Byte
    ASSERT_NE(p, nullptr);
    auto const* bytes = static_cast<unsigned char*>(p);
    for (std::size_t i = 0; i < 32; ++i) { EXPECT_EQ(bytes[i], 0u) << "Byte " << i << " sollte 0 sein"; }
    std::free(p); // calloc-allocated -> std::free OK (NICHT _aligned_free)
}

TEST(V41_TopicAllocatorAxis06, ReallocateGrowsAllocation) {
    axis_06::StdMalloc m{};
    void*              p = m.allocate(16, alignof(std::max_align_t));
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
    auto*              base_ref = static_cast<axis_06::AllocatorStrategyBase<axis_06::StdMalloc>*>(&m);

    void* p = base_ref->allocate(256, 16);
    ASSERT_NE(p, nullptr);
    base_ref->deallocate(p, 256, 16);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto stats = base_ref->statistics();
    EXPECT_EQ(stats.allocation_count, 1u);
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
    void*                      p = m.allocate(128, 16);
    ASSERT_NE(p, nullptr);
    m.deallocate(p, 128, 16);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    EXPECT_EQ(m.statistics().allocation_count, 1u);
    EXPECT_EQ(m.statistics().deallocation_count, 1u);
#endif
}

TEST(V41_TopicAllocatorAxis06, MimallocCollectIsCallable) {
    axis_06::MimallocAllocator m{};
    void*                      p = m.allocate(64, 8);
    m.deallocate(p, 64, 8);
    m.collect(true); // Reclaim-API darf nicht crashen
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
    void*                      p = s.allocate(256, 32);
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
    axis_06::PmrResourceAllocator p{}; // default = std::pmr::new_delete_resource()
    void*                         mem = p.allocate(64, 8);
    ASSERT_NE(mem, nullptr);
    p.deallocate(mem, 64, 8);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    EXPECT_EQ(p.statistics().allocation_count, 1u);
#endif
}

TEST(V41_TopicAllocatorAxis06, PmrResourceWithCustomResource) {
    // Test mit explicit monotonic_buffer_resource (kein delete bis Reset)
    std::byte                           buffer[1024];
    std::pmr::monotonic_buffer_resource mono(buffer, sizeof(buffer));
    axis_06::PmrResourceAllocator       p{&mono};
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
    static_assert(std::is_same_v<decltype(flags::std_enabled), const bool>);
    static_assert(std::is_same_v<decltype(flags::mimalloc_enabled), const bool>);
    static_assert(std::is_same_v<decltype(flags::snmalloc_enabled), const bool>);
    static_assert(std::is_same_v<decltype(flags::pmr_enabled), const bool>);
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, RegistryAllVendorsCount) {
    using AllV = axis_06::AllVendors;
    // KEIN hartkodierter Count — User-Direktive 2026-05-26: bei Batch-Erweiterung
    // ist `AllVendorsCountUpToDate` (dynamisch >= 4) ausreichend, kein Manual-Update noetig.
    constexpr auto n = boost::mp11::mp_size<AllV>::value;
    EXPECT_GE(n, 4u) << "mindestens Batch 1 (4 Vendor)";
    SUCCEED();
}

TEST(V41_TopicAllocatorAxis06, RegistryEnabledVendorsNonEmpty) {
    using EnabledV = axis_06::EnabledVendors;
    // Stufe 1: is_enabled<T> = mp_bool<true> -> EnabledVendors = AllVendors
    static_assert(boost::mp11::mp_size<EnabledV>::value > 0, "Mindestens 1 Vendor muss enabled sein");
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
    EXPECT_GE(expected, 1u); // mindestens STD/PMR (immer verfuegbar)
}

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1.I Stufe 3 LIVE: Observer-Notify-Pattern (Pflicht wenn STATISTICS=ON)
// ───────────────────────────────────────────────────────────────────────────
#ifdef COMDARE_CE_ENABLE_STATISTICS

// (a) MeasurableComponent Concept erfuellt — observer_t als MeasurableObserver<snapshot_t>
TEST(V41_TopicAllocatorAxis06_Stufe3, WrapperHasObserverAlias) {
    using ObserverT_Std = axis_06::StdMalloc::observer_t;
    using SnapshotT_Std = axis_06::StdMalloc::snapshot_t;
    static_assert(
        std::is_same_v<ObserverT_Std, ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Std>>,
        "StdMalloc::observer_t muss MeasurableObserver<snapshot_t> sein");

    using ObserverT_Mi = axis_06::MimallocAllocator::observer_t;
    using SnapshotT_Mi = axis_06::MimallocAllocator::snapshot_t;
    static_assert(std::is_same_v<ObserverT_Mi, ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Mi>>,
                  "MimallocAllocator::observer_t muss MeasurableObserver<snapshot_t> sein");

    using ObserverT_Sn = axis_06::SnmallocAllocator::observer_t;
    using SnapshotT_Sn = axis_06::SnmallocAllocator::snapshot_t;
    static_assert(std::is_same_v<ObserverT_Sn, ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Sn>>,
                  "SnmallocAllocator::observer_t muss MeasurableObserver<snapshot_t> sein");

    using ObserverT_Pmr = axis_06::PmrResourceAllocator::observer_t;
    using SnapshotT_Pmr = axis_06::PmrResourceAllocator::snapshot_t;
    static_assert(
        std::is_same_v<ObserverT_Pmr, ::comdare::cache_engine::measurement::MeasurableObserver<SnapshotT_Pmr>>,
        "PmrResourceAllocator::observer_t muss MeasurableObserver<snapshot_t> sein");
    SUCCEED();
}

// (b) Observer-Callback wird bei allocate() benachrichtigt
TEST(V41_TopicAllocatorAxis06_Stufe3, ObserverNotifiesOnAllocate_StdMalloc) {
    axis_06::StdMalloc m{};
    int                events     = 0;
    std::uint64_t      last_count = 0;
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
    int                events = 0;
    m.observer().on_event([&events](auto const&) { ++events; });

    // zero_allocate -> std::free Pfad (separater Lifecycle)
    void* zp = m.zero_allocate(4, 16);
    ASSERT_NE(zp, nullptr);
    EXPECT_EQ(events, 1);
    std::free(zp); // zero_allocate verwendet std::calloc — passendes std::free, KEIN reallocate

    // reallocate ueber portable_aligned_alloc Lifecycle (separat)
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(events, 2);
    void* np = m.reallocate(p, 64, 128, 8);
    ASSERT_NE(np, nullptr);
    EXPECT_GE(events, 3); // reallocate macht 1 notify im Erfolgs-Fall

    m.deallocate(np, 128, 8);
    EXPECT_GE(events, 4);
}

// (d) reset() benachrichtigt Observer mit leeren Stats
TEST(V41_TopicAllocatorAxis06_Stufe3, ResetNotifiesObserverWithClearedStats) {
    axis_06::StdMalloc m{};
    void*              p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr);

    std::uint64_t last_alloc_count = 0;
    m.observer().on_event([&last_alloc_count](auto const& snap) { last_alloc_count = snap.allocation_count; });
    m.reset();
    EXPECT_EQ(last_alloc_count, 0u);

    m.deallocate(p, 64, 8);
}

// (e) Observer ohne registriertes Callback -> notify ist no-op (kein Crash)
TEST(V41_TopicAllocatorAxis06_Stufe3, ObserverWithoutCallbackIsNoOp) {
    axis_06::StdMalloc m{};
    EXPECT_FALSE(m.observer().has_callback());
    void* p = m.allocate(64, 8);
    ASSERT_NE(p, nullptr); // kein Crash trotz fehlendem Callback
    m.deallocate(p, 64, 8);
    SUCCEED();
}

// (f) ObserverNotifiesOnAllocate_OtherVendors entfernt (2026-05-26)
// User-Direktive: redundant — TYPED_TEST(AllocatorVendorTest, ObserverNotifyOnAllocateAndDeallocate)
// am Anfang dieses Files deckt alle Vendor in AllVendors automatisch ab.

#endif // COMDARE_CE_ENABLE_STATISTICS

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1.D Stufe 4: PermutationEngine + TopicConfigSet (Doku §15.4 / §15.7)
// ───────────────────────────────────────────────────────────────────────────

namespace perms = comdare::cache_engine::permutations;
namespace alloc = comdare::cache_engine::allocator;

// (a) TopicConfigSet hat StaticAxisVariants = EnabledVendors
TEST(V41_TopicAllocatorAxis06_Stufe4, TopicConfigSetHasEnabledVendors) {
    using TCS = alloc::TopicConfigSet;
    static_assert(std::is_same_v<typename TCS::StaticAxisVariants, alloc::axis_06_allocator::EnabledVendors>,
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
    int seen     = 0;
    Engine::for_each_permutation([&seen]<class P>() {
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
    using P_Std = perms::PermTuple<axis_06::StdMalloc>;
    using P_Mi  = perms::PermTuple<axis_06::MimallocAllocator>;
    using P_Pmr = perms::PermTuple<axis_06::PmrResourceAllocator>;

    constexpr auto h_std = P_Std::hash();
    constexpr auto h_mi  = P_Mi::hash();
    constexpr auto h_pmr = P_Pmr::hash();

    EXPECT_NE(h_std, h_mi);
    EXPECT_NE(h_std, h_pmr);
    EXPECT_NE(h_mi, h_pmr);

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
    using Engine       = perms::PermutationEngine<alloc::TopicConfigSet>;
    int filtered_count = 0;
    Engine::for_each_filtered<perms::AlwaysTrue>([&filtered_count]<class>() { ++filtered_count; });
    EXPECT_EQ(static_cast<std::size_t>(filtered_count), Engine::count());
}

// (g) for_each_filtered mit AlwaysFalse = 0 Aufrufe
TEST(V41_TopicAllocatorAxis06_Stufe4, FilterAlwaysFalseIsZero) {
    using Engine       = perms::PermutationEngine<alloc::TopicConfigSet>;
    int filtered_count = 0;
    Engine::for_each_filtered<perms::AlwaysFalse>([&filtered_count]<class>() { ++filtered_count; });
    EXPECT_EQ(filtered_count, 0);
    static_assert(Engine::count_filtered<perms::AlwaysFalse>() == 0, "Diagnose-Counter mit AlwaysFalse muss 0 sein");
}

// (h) AxisFullJoin (Stufe 3 Skelett): mp_append + mp_unique
TEST(V41_TopicAllocatorAxis06_Stufe4, AxisFullJoinDeduplicatesVariants) {
    namespace mp = boost::mp11;
    // Beispiel: cache-engine Defaults + 1 Pruefling der StdMalloc auch nutzt
    using DefaultList   = mp::mp_list<axis_06::StdMalloc, axis_06::MimallocAllocator>;
    using PrueflingList = mp::mp_list<axis_06::StdMalloc, axis_06::SnmallocAllocator>; // StdMalloc doppelt
    using Joined        = perms::AxisFullJoin<DefaultList, PrueflingList>;
    // mp_unique entfernt das doppelte StdMalloc -> 3 statt 4
    static_assert(mp::mp_size<Joined>::value == 3, "AxisFullJoin muss Duplikate via mp_unique entfernen");
    SUCCEED();
}

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1.E iterable_aspect_t — Hybride Laufzeit-Permutation (Doku §15.5)
// ───────────────────────────────────────────────────────────────────────────

// Mock-Vendor mit iterable_aspect_t (nur fuer Tests, kein Production-Code)
struct ThresholdedAllocatorMock {
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5>   values{16u, 64u, 256u, 1024u, 4096u};
    static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{values.data(), values.size()};
    }
    static constexpr std::string_view name() noexcept { return "threshold_mock"; }
};

// Mock-Vendor OHNE iterable_aspect_t (Standard-Pfad)
struct PlainAllocatorMock {
    static constexpr std::string_view name() noexcept { return "plain_mock"; }
};

// (a) HasIterableAspect Concept erkennt korrekt
TEST(V41_TopicAllocatorAxis06_Stufe5, HasIterableAspectConceptDetection) {
    static_assert(perms::HasIterableAspect<ThresholdedAllocatorMock>,
                  "ThresholdedAllocatorMock erfuellt HasIterableAspect");
    static_assert(!perms::HasIterableAspect<PlainAllocatorMock>, "PlainAllocatorMock erfuellt HasIterableAspect NICHT");
    // Echte Wrapper haben heute KEIN iterable_aspect_t
    static_assert(!perms::HasIterableAspect<axis_06::StdMalloc>, "StdMalloc hat heute keinen iterable_aspect_t");
    SUCCEED();
}

// (b) aspect_count<V>() liefert 1 fuer Plain, N fuer Iterable
TEST(V41_TopicAllocatorAxis06_Stufe5, AspectCountForPlainAndIterable) {
    static_assert(perms::aspect_count<PlainAllocatorMock>() == 1u, "Plain Vendor hat aspect_count 1 (Default)");
    static_assert(perms::aspect_count<ThresholdedAllocatorMock>() == 5u, "Thresholded Vendor hat 5 iterable_values");
    EXPECT_EQ(perms::aspect_count<axis_06::StdMalloc>(), 1u);
}

// (c) for_each_aspect ueber Iterable: 5 Iterationen mit unterschiedlichen Werten
TEST(V41_TopicAllocatorAxis06_Stufe5, ForEachAspectIteratesAllValues) {
    std::vector<std::size_t> seen;
    perms::for_each_aspect<ThresholdedAllocatorMock>([&seen](std::size_t v) { seen.push_back(v); });
    ASSERT_EQ(seen.size(), 5u);
    EXPECT_EQ(seen[0], 16u);
    EXPECT_EQ(seen[1], 64u);
    EXPECT_EQ(seen[2], 256u);
    EXPECT_EQ(seen[3], 1024u);
    EXPECT_EQ(seen[4], 4096u);
}

// (d) for_each_aspect ueber Plain: 1 Aufruf ohne Argument
TEST(V41_TopicAllocatorAxis06_Stufe5, ForEachAspectPlainSingleCall) {
    int call_count = 0;
    perms::for_each_aspect<PlainAllocatorMock>([&call_count]() { ++call_count; });
    EXPECT_EQ(call_count, 1);
}

// (e) Hybride Mess-Reihe Demo: pro statische Permutation N dynamische Aspekt-Iterationen
TEST(V41_TopicAllocatorAxis06_Stufe5, HybridStaticAndDynamicCombo) {
    // Realer Use-Case: PermutationEngine iteriert statisch, innerhalb jeder
    // Iteration wird for_each_aspect runtime-mal aufgerufen.
    using Engine           = perms::PermutationEngine<alloc::TopicConfigSet>;
    int total_measurements = 0;

    Engine::for_each_permutation([&total_measurements]<class P>() {
        // P::variants ist mp_list aus Achs-Vendors. Hier nur 1 Achse (allocator),
        // also wir nehmen den ersten Vendor und iterieren seinen Aspekt.
        using FirstVendor = boost::mp11::mp_at_c<typename P::variants, 0>;
        perms::for_each_aspect<FirstVendor>([&total_measurements](auto...) { ++total_measurements; });
    });

    // Erwartete Total-Mess-Reihen = sum_over_permutations(aspect_count_of_each)
    // Heute alle Vendor ohne iterable_aspect_t -> aspect_count == 1 pro Vendor
    // -> total == Engine::count()
    EXPECT_EQ(static_cast<std::size_t>(total_measurements), Engine::count());
}

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1.G CacheEngineBuilder CLI-Flag-Builder (Doku §15.10)
// ───────────────────────────────────────────────────────────────────────────

// (a) Wrapper flag_suffix() ist Pflicht-Konvention
TEST(V41_TopicAllocatorAxis06_Stufe6, WrapperHasFlagSuffix) {
    static_assert(axis_06::StdMalloc::flag_suffix() == "STD");
    static_assert(axis_06::MimallocAllocator::flag_suffix() == "MIMALLOC");
    static_assert(axis_06::SnmallocAllocator::flag_suffix() == "SNMALLOC");
    static_assert(axis_06::PmrResourceAllocator::flag_suffix() == "PMR");
    SUCCEED();
}

// (b) hash_to_hex liefert 16-Zeichen Hex
TEST(V41_TopicAllocatorAxis06_Stufe6, HashToHexFormat) {
    std::string s = perms::hash_to_hex(0x0123456789abcdefULL);
    EXPECT_EQ(s, "0123456789abcdef");
    EXPECT_EQ(s.size(), 16u);

    std::string z = perms::hash_to_hex(0ULL);
    EXPECT_EQ(z, "0000000000000000");
}

// (c) build_cmake_invocation_prefix mit deterministischem Hash
TEST(V41_TopicAllocatorAxis06_Stufe6, CmakeInvocationPrefix) {
    using P         = perms::PermTuple<axis_06::StdMalloc>;
    std::string cmd = perms::build_cmake_invocation_prefix<P>("/src", "/out");
    EXPECT_TRUE(cmd.starts_with("cmake -B /out/perm_"));
    EXPECT_TRUE(cmd.ends_with(" -S /src"));
    // Hash-Anteil ist 16 Hex-Zeichen lang
    std::string hash_part = perms::hash_to_hex(P::hash());
    EXPECT_NE(cmd.find(hash_part), std::string::npos);
}

// (d) emit_axis_flags: StdMalloc selected = STD=ON, andere=OFF
TEST(V41_TopicAllocatorAxis06_Stufe6, EmitAxisFlagsStdSelected) {
    std::string flags = perms::emit_axis_flags<axis_06::StdMalloc, axis_06::AllVendors>("COMDARE_AXIS_06_ENABLE");
    EXPECT_NE(flags.find(" -DCOMDARE_AXIS_06_ENABLE_STD=ON"), std::string::npos);
    EXPECT_NE(flags.find(" -DCOMDARE_AXIS_06_ENABLE_MIMALLOC=OFF"), std::string::npos);
    EXPECT_NE(flags.find(" -DCOMDARE_AXIS_06_ENABLE_SNMALLOC=OFF"), std::string::npos);
    EXPECT_NE(flags.find(" -DCOMDARE_AXIS_06_ENABLE_PMR=OFF"), std::string::npos);
    // STD nicht =OFF
    EXPECT_EQ(flags.find(" -DCOMDARE_AXIS_06_ENABLE_STD=OFF"), std::string::npos);
}

// (e) emit_axis_flags: Mimalloc selected
TEST(V41_TopicAllocatorAxis06_Stufe6, EmitAxisFlagsMimallocSelected) {
    std::string flags =
        perms::emit_axis_flags<axis_06::MimallocAllocator, axis_06::AllVendors>("COMDARE_AXIS_06_ENABLE");
    EXPECT_NE(flags.find(" -DCOMDARE_AXIS_06_ENABLE_MIMALLOC=ON"), std::string::npos);
    EXPECT_NE(flags.find(" -DCOMDARE_AXIS_06_ENABLE_STD=OFF"), std::string::npos);
}

// (f) build_cmake_command_for_single_topic: voller Befehl
TEST(V41_TopicAllocatorAxis06_Stufe6, FullCmakeCommandSingleTopic) {
    using P_Std     = perms::PermTuple<axis_06::StdMalloc>;
    std::string cmd = perms::build_cmake_command_for_single_topic<P_Std, axis_06::AllVendors>("/src", "/out",
                                                                                              "COMDARE_AXIS_06_ENABLE");

    EXPECT_TRUE(cmd.starts_with("cmake -B /out/perm_"));
    EXPECT_NE(cmd.find(" -S /src"), std::string::npos);
    EXPECT_NE(cmd.find(" -DCOMDARE_AXIS_06_ENABLE_STD=ON"), std::string::npos);
    EXPECT_NE(cmd.find(" -DCOMDARE_AXIS_06_ENABLE_PMR=OFF"), std::string::npos);
}

// (g) Demo: PermutationEngine + CLI-Builder zusammen — alle CMake-Commands pro Permutation
TEST(V41_TopicAllocatorAxis06_Stufe6, EngineAndCliBuilderCombo) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    std::vector<std::string> commands;
    Engine::for_each_permutation([&commands]<class P>() {
        commands.push_back(perms::build_cmake_command_for_single_topic<P, axis_06::AllVendors>(
            "/src", "/out", "COMDARE_AXIS_06_ENABLE"));
    });
    EXPECT_EQ(commands.size(), Engine::count());
    // Jeder Command beginnt mit cmake -B
    for (auto const& c : commands) { EXPECT_TRUE(c.starts_with("cmake -B ")); }
}

// ───────────────────────────────────────────────────────────────────────────
// V41.F.6.1 Batch 2+ Vendor-Spezifika (per-Vendor Tests via TYPED_TEST_SUITE
// am Anfang dieses Files — User-Direktive 2026-05-26)
// ───────────────────────────────────────────────────────────────────────────
// ConceptConformance / Identification / AllocateDeallocateRoundtrip / ObserverNotify
// werden jetzt AUTOMATISCH pro Vendor in AllVendors expandiert. Vendor-Erweiterung
// (Batch 3-8) addiert KEINE Tests im Test-File mehr — nur Registry-Update reicht.
//
// Hier verbleibt nur die AllVendors-Count-Sanity (manuell ablesbar):
TEST(V41_TopicAllocatorAxis06_Registry, AllVendorsCountUpToDate) {
    constexpr auto n = boost::mp11::mp_size<axis_06::AllVendors>::value;
    EXPECT_GE(n, 4u) << "mindestens Batch 1 (Std/Mi/Sn/PMR)";
    SUCCEED();
}

// (f) PermutationEngine count beruecksichtigt Batch 2 (Cartesian-Erweiterung)
TEST(V41_TopicAllocatorAxis06_Batch2, EngineCountIncludesBatch2) {
    using Engine = perms::PermutationEngine<alloc::TopicConfigSet>;
    // EnabledVendors haengt von CMake-Flags ab. build-pilot: Std + PMR + Je + Tc + Dl
    // (HAVE=0 fuer Mimalloc/Snmalloc, aber JE/TC/DL haben jetzt HAVE=0 default — siehe nachsten Test)
    constexpr auto cnt = Engine::count();
    EXPECT_GE(cnt, 1u); // mindestens Std + PMR
    // EnabledVendors muss == AllVendors haben WENN alle ENABLE=ON UND HAVE=ON (production build)
    // build-pilot: nur Std + PMR HAVE=ON (Batch 2 alle HAVE=OFF → USE=OFF)
}

// =================================================================
// V41.F.6.1.R7.4 — Adapter-Methoden as_std_allocator<T>() + as_pmr_resource()
// (vorher static_assert-Stubs). Getestet ueber StdMalloc-Baseline (dependency-frei).
// =================================================================

TEST(V41_TopicAllocatorAxis06_Adapter, AsStdAllocatorBacksStdVector) {
    axis_06::StdMalloc                a{};
    auto                              alloc = a.as_std_allocator<int>();
    std::vector<int, decltype(alloc)> v{alloc};
    for (int i = 0; i < 1000; ++i) v.push_back(i); // erzwingt mehrere Reallocs via Achsen-Strategie
    ASSERT_EQ(v.size(), 1000u);
    EXPECT_EQ(v.front(), 0);
    EXPECT_EQ(v.back(), 999);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0LL), 999LL * 1000LL / 2LL);
}

TEST(V41_TopicAllocatorAxis06_Adapter, StdAllocatorRebindAndEquality) {
    axis_06::StdMalloc a{};
    auto               ai = a.as_std_allocator<int>();
    // Rebind int -> double via allocator_traits + Converting-Ctor
    using AllocDouble = std::allocator_traits<decltype(ai)>::rebind_alloc<double>;
    AllocDouble ad{ai};
    EXPECT_TRUE(ai == ad); // gleiche zugrundeliegende Strategie
    axis_06::StdMalloc b{};
    auto               bi = b.as_std_allocator<int>();
    EXPECT_FALSE(ai == bi); // verschiedene Strategie-Instanzen
}

TEST(V41_TopicAllocatorAxis06_Adapter, AsPmrResourceBacksPmrVector) {
    axis_06::StdMalloc              a{};
    auto                            res = a.as_pmr_resource(); // Wert; Aufrufer haelt ihn am Leben
    std::pmr::vector<std::uint64_t> v{&res};
    for (std::uint64_t i = 0; i < 500; ++i) v.push_back(i * 2u);
    ASSERT_EQ(v.size(), 500u);
    EXPECT_EQ(v[10], 20u);
    EXPECT_EQ(v.back(), 998u);
    // do_is_equal: dieselbe Strategie → gleich; andere Strategie → ungleich.
    auto               res_same = a.as_pmr_resource();
    axis_06::StdMalloc b{};
    auto               res_other = b.as_pmr_resource();
    EXPECT_TRUE(res.is_equal(res_same));
    EXPECT_FALSE(res.is_equal(res_other));
}

// V41.F.6.1 R5.B (2026-05-29) — PoolResourceAllocator: die NICHT-HOHL-Eigenschaft.
// Anders als die uebrigen axis_06-Wrapper (die ohne Vendor-Linking auf System-malloc zurueckfallen)
// besitzt dieser einen EIGENEN std::pmr::unsynchronized_pool_resource → verhaltens-distinkt.
TEST(V41_TopicAllocatorAxis06_Pool, OwnsDistinctPoolAndRoundtrips) {
    axis_06::PoolResourceAllocator a{};
    EXPECT_TRUE(axis_06::PoolResourceAllocator::supports_pmr());
    EXPECT_FALSE(axis_06::PoolResourceAllocator::is_thread_safe()); // unsynchronized
    EXPECT_EQ(axis_06::PoolResourceAllocator::name(), std::string_view{"pool_resource"});
    ASSERT_NE(a.underlying_resource(), nullptr);

    // Pool-Roundtrip mit vielen kleinen, gleichgrossen Allokationen (Size-Class-Wiederverwendung).
    constexpr std::size_t kBytes = 64, kAlign = 16, kN = 1000;
    std::vector<void*>    ptrs;
    ptrs.reserve(kN);
    for (std::size_t i = 0; i < kN; ++i) {
        void* p = a.allocate(kBytes, kAlign);
        ASSERT_NE(p, nullptr);
        std::memset(p, 0xAB, kBytes); // Speicher wirklich anfassen
        ptrs.push_back(p);
    }
    for (void* p : ptrs) a.deallocate(p, kBytes, kAlign); // zurueck in die Pool-Free-Lists
    // Nach Freigabe erneut allokieren → Pool bedient aus wiederverwendeten Bloecken (kein Crash).
    void* reuse = a.allocate(kBytes, kAlign);
    ASSERT_NE(reuse, nullptr);
    a.deallocate(reuse, kBytes, kAlign);
}

// is_equal-Semantik: distinkte Default-Instanzen besitzen UNTERSCHIEDLICHE Pools (ungleich);
// eine Kopie TEILT den Pool (gleich) — korrekte PMR-Semantik fuer einen stateful Allocator.
TEST(V41_TopicAllocatorAxis06_Pool, CopySharesPoolDistinctInstancesDiffer) {
    axis_06::PoolResourceAllocator a{};
    axis_06::PoolResourceAllocator a_copy{a}; // teilt den shared_ptr-Pool
    axis_06::PoolResourceAllocator b{};       // eigener, anderer Pool
    EXPECT_TRUE(a == a_copy);                 // gleicher Pool
    EXPECT_FALSE(a == b);                     // verschiedene Pools
    EXPECT_EQ(a.underlying_resource(), a_copy.underlying_resource());
    EXPECT_NE(a.underlying_resource(), b.underlying_resource());
}
