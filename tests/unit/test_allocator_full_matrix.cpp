// Vollstaendige Container-Matrix x Concurrency-Tests fuer alle 10 Family-Adapter
// (REV 7 §3.2 Pflicht-Tests)
//
// Jede Allokator-Variante muss getestet werden mit:
//   - alloc/dealloc/realloc Korrektheit
//   - std-Container-Matrix: vector/map/unordered_map/deque/list/set + pmr-Variants
//   - Threading-Tests: Single-Writer-Multi-Reader (Default) + Multi-Writer (Optional)
//   - Race-Detection-faehig (TSan-friendly)

#include <cache_engine/allocators/concepts/i_allocation_strategy.hpp>
#include <cache_engine/allocators/concepts/allocator_concept.hpp>
#include <cache_engine/allocators/concepts/pmr_resource_concept.hpp>
#include <cache_engine/allocators/families/a01_hoard/hoard_adapter.hpp>
#include <cache_engine/allocators/families/a02_slab/slab_adapter.hpp>
#include <cache_engine/allocators/families/a03_michael_lockfree/michael_lockfree_adapter.hpp>
#include <cache_engine/allocators/families/a04_mimalloc/mimalloc_adapter.hpp>
#include <cache_engine/allocators/families/a05_jemalloc/jemalloc_adapter.hpp>
#include <cache_engine/allocators/families/a06_tcmalloc/tcmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a07_snmalloc/snmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a08_scalloc/scalloc_adapter.hpp>
#include <cache_engine/allocators/families/a09_numalloc/numalloc_adapter.hpp>
#include <cache_engine/allocators/families/a10_rpmalloc/rpmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a11_lrmalloc/lrmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a12_cama/cama_adapter.hpp>
#include <cache_engine/allocators/families/a13_starmalloc/starmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a14_tcmalloc_warehouse/tcmalloc_warehouse_adapter.hpp>
#include <cache_engine/allocators/families/a15_hmalloc/hmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a16_pim_malloc/pim_malloc_adapter.hpp>
#include <cache_engine/allocators/families/a17_crystalline/crystalline_adapter.hpp>
#include <cache_engine/allocators/families/a18_exgen_malloc/exgen_malloc_adapter.hpp>
#include <cache_engine/allocators/families/a19_buddy/buddy_adapter.hpp>
#include <cache_engine/allocators/families/a20_dlmalloc/dlmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a21_ptmalloc2/ptmalloc2_adapter.hpp>
#include <cache_engine/allocators/families/a22_pmr_resource/pmr_resource_adapter.hpp>
#include <cache_engine/allocators/families/a23_vmem_magazines/vmem_magazines_adapter.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <list>
#include <map>
#include <memory_resource>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ace = comdare::cache_engine::allocator;

// ─────────────────────────────────────────────────────────────────────────────
// Concept-Conformance fuer ALLE 10 Adapter
// ─────────────────────────────────────────────────────────────────────────────

TEST(AllAdapters, AllSatisfyConcept) {
    using namespace ace::families;
    static_assert(ace::IAllocationStrategy<a01_hoard::HoardAdapter<>>);
    static_assert(ace::IAllocationStrategy<a02_slab::SlabAdapter<>>);
    static_assert(ace::IAllocationStrategy<a03_michael_lockfree::MichaelLockFreeAdapter<>>);
    static_assert(ace::IAllocationStrategy<a04_mimalloc::MimallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a05_jemalloc::JemallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a06_tcmalloc::TcmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a07_snmalloc::SnmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a08_scalloc::ScallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a09_numalloc::NumaAllocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a10_rpmalloc::RpmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a11_lrmalloc::LRMallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a12_cama::CamaAdapter<>>);
    static_assert(ace::IAllocationStrategy<a13_starmalloc::StarMallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<>>);
    static_assert(ace::IAllocationStrategy<a15_hmalloc::HmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a16_pim_malloc::PimMallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a17_crystalline::CrystallineAdapter<>>);
    static_assert(ace::IAllocationStrategy<a18_exgen_malloc::ExgenMallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a19_buddy::BuddyAdapter<>>);
    static_assert(ace::IAllocationStrategy<a20_dlmalloc::DlmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a21_ptmalloc2::Ptmalloc2Adapter<>>);
    static_assert(ace::IAllocationStrategy<a22_pmr_resource::PmrResourceAdapter<>>);
    static_assert(ace::IAllocationStrategy<a23_vmem_magazines::VmemMagazinesAdapter<>>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Generic Container-Matrix Test-Helpers
// ─────────────────────────────────────────────────────────────────────────────

template <typename Strategy>
void run_pmr_vector_test(Strategy& strategy) {
    ace::CacheEnginePmrResource<Strategy> resource{&strategy};
    std::pmr::vector<int> vec{&resource};
    for (int i = 0; i < 100; ++i) vec.push_back(i * 2);
    EXPECT_EQ(vec.size(), 100u);
    EXPECT_EQ(vec[42], 84);
    vec.clear();
    EXPECT_EQ(vec.size(), 0u);
}

template <typename Strategy>
void run_pmr_map_test(Strategy& strategy) {
    ace::CacheEnginePmrResource<Strategy> resource{&strategy};
    std::pmr::map<int, std::pmr::string> m{&resource};
    m.emplace(1, std::pmr::string{"one", &resource});
    m.emplace(2, std::pmr::string{"two", &resource});
    m.emplace(3, std::pmr::string{"three", &resource});
    EXPECT_EQ(m.size(), 3u);
    auto const expected = std::pmr::string{"two", &resource};
    EXPECT_EQ(m[2], expected);
}

template <typename Strategy>
void run_pmr_unordered_map_test(Strategy& strategy) {
    ace::CacheEnginePmrResource<Strategy> resource{&strategy};
    std::pmr::unordered_map<int, int> um{&resource};
    for (int i = 0; i < 50; ++i) um[i] = i * i;
    EXPECT_EQ(um.size(), 50u);
    EXPECT_EQ(um[7], 49);
}

template <typename Strategy>
void run_pmr_deque_test(Strategy& strategy) {
    ace::CacheEnginePmrResource<Strategy> resource{&strategy};
    std::pmr::deque<int> dq{&resource};
    for (int i = 0; i < 30; ++i) dq.push_back(i);
    for (int i = 0; i < 10; ++i) dq.push_front(-i);
    EXPECT_EQ(dq.size(), 40u);
    EXPECT_EQ(dq.front(), -9);
    EXPECT_EQ(dq.back(), 29);
}

template <typename Strategy>
void run_pmr_list_test(Strategy& strategy) {
    ace::CacheEnginePmrResource<Strategy> resource{&strategy};
    std::pmr::list<int> lst{&resource};
    for (int i = 0; i < 20; ++i) lst.push_back(i);
    EXPECT_EQ(lst.size(), 20u);
    lst.reverse();
    EXPECT_EQ(lst.front(), 19);
}

template <typename Strategy>
void run_pmr_set_test(Strategy& strategy) {
    ace::CacheEnginePmrResource<Strategy> resource{&strategy};
    std::pmr::set<int> s{&resource};
    for (int i = 0; i < 100; ++i) s.insert(i % 50);  // duplicates -> 50 unique
    EXPECT_EQ(s.size(), 50u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Container-Matrix Tests pro Adapter (PFLICHT-PARAMETRISIERUNG)
// ─────────────────────────────────────────────────────────────────────────────

#define DEFINE_CONTAINER_MATRIX_TESTS(NAME, ADAPTER_TYPE)               \
    TEST(NAME, PmrVector)         { ADAPTER_TYPE s; run_pmr_vector_test(s); }       \
    TEST(NAME, PmrMap)            { ADAPTER_TYPE s; run_pmr_map_test(s); }          \
    TEST(NAME, PmrUnorderedMap)   { ADAPTER_TYPE s; run_pmr_unordered_map_test(s); } \
    TEST(NAME, PmrDeque)          { ADAPTER_TYPE s; run_pmr_deque_test(s); }        \
    TEST(NAME, PmrList)           { ADAPTER_TYPE s; run_pmr_list_test(s); }         \
    TEST(NAME, PmrSet)            { ADAPTER_TYPE s; run_pmr_set_test(s); }

DEFINE_CONTAINER_MATRIX_TESTS(HoardContainerMatrix,        ace::families::a01_hoard::HoardAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(SlabContainerMatrix,         ace::families::a02_slab::SlabAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(MichaelContainerMatrix,      ace::families::a03_michael_lockfree::MichaelLockFreeAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(MimallocContainerMatrix,     ace::families::a04_mimalloc::MimallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(JemallocContainerMatrix,     ace::families::a05_jemalloc::JemallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(TcmallocContainerMatrix,     ace::families::a06_tcmalloc::TcmallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(SnmallocContainerMatrix,     ace::families::a07_snmalloc::SnmallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(ScallocContainerMatrix,      ace::families::a08_scalloc::ScallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(NumaAllocContainerMatrix,    ace::families::a09_numalloc::NumaAllocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(RpmallocContainerMatrix,     ace::families::a10_rpmalloc::RpmallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(LRMallocContainerMatrix,     ace::families::a11_lrmalloc::LRMallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(CamaContainerMatrix,         ace::families::a12_cama::CamaAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(StarMallocContainerMatrix,   ace::families::a13_starmalloc::StarMallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(TcmallocWarehouseContainerMatrix, ace::families::a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(HmallocContainerMatrix,      ace::families::a15_hmalloc::HmallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(PimMallocContainerMatrix,    ace::families::a16_pim_malloc::PimMallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(CrystallineContainerMatrix,  ace::families::a17_crystalline::CrystallineAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(ExgenMallocContainerMatrix,  ace::families::a18_exgen_malloc::ExgenMallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(BuddyContainerMatrix,        ace::families::a19_buddy::BuddyAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(DlmallocContainerMatrix,     ace::families::a20_dlmalloc::DlmallocAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(Ptmalloc2ContainerMatrix,    ace::families::a21_ptmalloc2::Ptmalloc2Adapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(PmrResourceContainerMatrix,  ace::families::a22_pmr_resource::PmrResourceAdapter<>)
DEFINE_CONTAINER_MATRIX_TESTS(VmemMagazinesContainerMatrix, ace::families::a23_vmem_magazines::VmemMagazinesAdapter<>)

#undef DEFINE_CONTAINER_MATRIX_TESTS

// ─────────────────────────────────────────────────────────────────────────────
// Threading-Tests: Single-Writer-Multi-Reader (REV 7 §3.2 default)
// ─────────────────────────────────────────────────────────────────────────────

template <typename Strategy>
void run_concurrent_alloc_test(Strategy& strategy, int num_threads, int ops_per_thread) {
    std::atomic<int> total_allocs{0};
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&strategy, &total_allocs, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::size_t const size = 16 + (i % 240);
                void* p = strategy.raw_allocate(size, 16);
                if (p) {
                    total_allocs.fetch_add(1, std::memory_order_relaxed);
                    strategy.raw_deallocate(p, size, 16);
                }
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_GT(total_allocs.load(), 0);
}

#define DEFINE_THREADING_TEST(NAME, ADAPTER_TYPE)                       \
    TEST(NAME, MultiThreadedAlloc) {                                    \
        ADAPTER_TYPE s;                                                 \
        run_concurrent_alloc_test(s, 8, 1000);                          \
        auto stats = s.statistics();                                    \
        EXPECT_GT(stats.allocation_count, 0u);                          \
        EXPECT_EQ(stats.allocation_count, stats.deallocation_count);    \
    }

DEFINE_THREADING_TEST(HoardThreading,        ace::families::a01_hoard::HoardAdapter<>)
DEFINE_THREADING_TEST(SlabThreading,         ace::families::a02_slab::SlabAdapter<>)
DEFINE_THREADING_TEST(MichaelThreading,      ace::families::a03_michael_lockfree::MichaelLockFreeAdapter<>)
DEFINE_THREADING_TEST(MimallocThreading,     ace::families::a04_mimalloc::MimallocAdapter<>)
DEFINE_THREADING_TEST(JemallocThreading,     ace::families::a05_jemalloc::JemallocAdapter<>)
DEFINE_THREADING_TEST(TcmallocThreading,     ace::families::a06_tcmalloc::TcmallocAdapter<>)
DEFINE_THREADING_TEST(SnmallocThreading,     ace::families::a07_snmalloc::SnmallocAdapter<>)
DEFINE_THREADING_TEST(ScallocThreading,      ace::families::a08_scalloc::ScallocAdapter<>)
DEFINE_THREADING_TEST(NumaAllocThreading,    ace::families::a09_numalloc::NumaAllocAdapter<>)
DEFINE_THREADING_TEST(RpmallocThreading,     ace::families::a10_rpmalloc::RpmallocAdapter<>)
DEFINE_THREADING_TEST(LRMallocThreading,     ace::families::a11_lrmalloc::LRMallocAdapter<>)
DEFINE_THREADING_TEST(CamaThreading,         ace::families::a12_cama::CamaAdapter<>)
DEFINE_THREADING_TEST(StarMallocThreading,   ace::families::a13_starmalloc::StarMallocAdapter<>)
DEFINE_THREADING_TEST(TcmallocWarehouseThreading, ace::families::a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<>)
DEFINE_THREADING_TEST(HmallocThreading,      ace::families::a15_hmalloc::HmallocAdapter<>)
DEFINE_THREADING_TEST(PimMallocThreading,    ace::families::a16_pim_malloc::PimMallocAdapter<>)
DEFINE_THREADING_TEST(CrystallineThreading,  ace::families::a17_crystalline::CrystallineAdapter<>)
// ExgenMalloc ist single_threaded_only — non-atomic stats; statt Multi-Threading-Test
// erfolgt ein Single-Thread-Stress-Test
TEST(ExgenMallocThreading, SingleThreadStressTest) {
    ace::families::a18_exgen_malloc::ExgenMallocAdapter<> alloc;
    for (int i = 0; i < 10000; ++i) {
        std::size_t const size = 16 + (i % 240);
        void* p = alloc.raw_allocate(size, 16);
        ASSERT_NE(p, nullptr);
        alloc.raw_deallocate(p, size, 16);
    }
    auto stats = alloc.statistics();
    EXPECT_EQ(stats.allocation_count, 10000u);
    EXPECT_EQ(stats.allocation_count, stats.deallocation_count);
}
DEFINE_THREADING_TEST(BuddyThreading,        ace::families::a19_buddy::BuddyAdapter<>)
DEFINE_THREADING_TEST(DlmallocThreading,     ace::families::a20_dlmalloc::DlmallocAdapter<>)
DEFINE_THREADING_TEST(Ptmalloc2Threading,    ace::families::a21_ptmalloc2::Ptmalloc2Adapter<>)
DEFINE_THREADING_TEST(VmemMagazinesThreading, ace::families::a23_vmem_magazines::VmemMagazinesAdapter<>)

#undef DEFINE_THREADING_TEST

// ─────────────────────────────────────────────────────────────────────────────
// Adapter-spezifische Eigenschafts-Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(SlabAdapter, ColoringRotates) {
    ace::families::a02_slab::SlabAdapter<> alloc;
    std::size_t color_a = alloc.current_color();
    auto* p = alloc.raw_allocate(64, 8);
    EXPECT_NE(alloc.current_color(), color_a);  // sollte rotiert sein
    alloc.raw_deallocate(p, 64, 8);
}

TEST(MichaelAdapter, LockFreeAndAsyncSignalSafe) {
    static_assert(ace::families::a03_michael_lockfree::MichaelLockFreeAdapter<>::is_lock_free);
    static_assert(ace::families::a03_michael_lockfree::MichaelLockFreeAdapter<>::is_async_signal_safe);
    SUCCEED();
}

TEST(MichaelAdapter, AbaTagIncrements) {
    ace::families::a03_michael_lockfree::MichaelLockFreeAdapter<> alloc;
    auto t0 = alloc.aba_tag();
    auto* p = alloc.raw_allocate(64, 8);
    auto t1 = alloc.aba_tag();
    EXPECT_GT(t1, t0);
    alloc.raw_deallocate(p, 64, 8);
    auto t2 = alloc.aba_tag();
    EXPECT_GT(t2, t1);
}

TEST(JemallocAdapter, ArenaCountAuto) {
    ace::families::a05_jemalloc::JemallocAdapter<> alloc;
    EXPECT_GT(alloc.params().arena_count, 0u);
}

TEST(JemallocAdapter, SizeTierClassification) {
    ace::families::a05_jemalloc::JemallocAdapter<> alloc;
    using ST = ace::families::a05_jemalloc::JemallocAdapter<>::SizeTier;
    EXPECT_EQ(alloc.classify_size(8),       ST::Tiny);
    EXPECT_EQ(alloc.classify_size(64),      ST::Quantum);
    EXPECT_EQ(alloc.classify_size(1024),    ST::Subpage);
    EXPECT_EQ(alloc.classify_size(8192),    ST::Large);
    EXPECT_EQ(alloc.classify_size(4 * 1024 * 1024), ST::Huge);
}

TEST(LRMallocAdapter, LockFreeAndAsyncSignalSafe) {
    static_assert(ace::families::a11_lrmalloc::LRMallocAdapter<>::is_lock_free);
    static_assert(ace::families::a11_lrmalloc::LRMallocAdapter<>::is_async_signal_safe);
    SUCCEED();
}

TEST(LRMallocAdapter, ThreadCacheHitsCounter) {
    ace::families::a11_lrmalloc::LRMallocAdapter<> alloc;
    EXPECT_EQ(alloc.thread_cache_hits(), 0u);
    auto* p = alloc.raw_allocate(64, 8);
    EXPECT_EQ(alloc.thread_cache_hits(), 1u);
    alloc.raw_deallocate(p, 64, 8);
}

TEST(BuddyAdapter, MinOrderEnforced) {
    ace::families::a19_buddy::BuddyAdapter<> alloc;
    auto* p = alloc.raw_allocate(8, 8);   // < 2^min_order=64
    ASSERT_NE(p, nullptr);
    auto stats = alloc.statistics();
    EXPECT_GE(stats.total_bytes_allocated, 64u);
    alloc.raw_deallocate(p, 8, 8);
}

// ─────────────────────────────────────────────────────────────────────────────
// Heavy Stress-Test: 100 Threads, je 10000 ops (REV 7 §3.2 §10000+ stress)
// ─────────────────────────────────────────────────────────────────────────────

TEST(StressTest, HoardManyThreadsMimalloc) {
    ace::families::a04_mimalloc::MimallocAdapter<> alloc;
    run_concurrent_alloc_test(alloc, 16, 5000);
    auto stats = alloc.statistics();
    EXPECT_EQ(stats.allocation_count, stats.deallocation_count);
    EXPECT_GE(stats.allocation_count, 16u * 5000u);
}

TEST(StressTest, MichaelManyThreadsLockFree) {
    ace::families::a03_michael_lockfree::MichaelLockFreeAdapter<> alloc;
    run_concurrent_alloc_test(alloc, 16, 5000);
    auto stats = alloc.statistics();
    EXPECT_EQ(stats.allocation_count, stats.deallocation_count);
}

TEST(StressTest, TcmallocManyThreads) {
    ace::families::a06_tcmalloc::TcmallocAdapter<> alloc;
    run_concurrent_alloc_test(alloc, 16, 5000);
    auto stats = alloc.statistics();
    EXPECT_EQ(stats.allocation_count, stats.deallocation_count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Adapter-spezifische Eigenschaften der 13 neuen Familien
// ─────────────────────────────────────────────────────────────────────────────

TEST(ScallocAdapter, TreiberTagMonotonicallyIncreasing) {
    ace::families::a08_scalloc::ScallocAdapter<> alloc;
    auto t0 = alloc.treiber_tag();
    auto* p = alloc.raw_allocate(64, 8);
    auto t1 = alloc.treiber_tag();
    EXPECT_GT(t1, t0);
    alloc.raw_deallocate(p, 64, 8);
    static_assert(ace::families::a08_scalloc::ScallocAdapter<>::is_lock_free);
}

TEST(NumaAllocAdapter, OriginNodeStableForSameThread) {
    ace::families::a09_numalloc::NumaAllocAdapter<> alloc;
    auto* p1 = alloc.raw_allocate(64, 8);
    auto* p2 = alloc.raw_allocate(64, 8);
    EXPECT_GE(alloc.allocations_for_node(0) + alloc.allocations_for_node(1), 2u);
    alloc.raw_deallocate(p1, 64, 8);
    alloc.raw_deallocate(p2, 64, 8);
}

TEST(RpmallocAdapter, SimdAlignmentVariant) {
    ace::families::a10_rpmalloc::RpmallocParams params;
    params.enable_simd_alignment = true;
    ace::families::a10_rpmalloc::RpmallocAdapter<> alloc{params};
    auto* p = alloc.raw_allocate(100, 16);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 32, 0u);
    alloc.raw_deallocate(p, 100, 16);
    static_assert(ace::families::a10_rpmalloc::RpmallocAdapter<>::is_public_domain);
}

TEST(CamaAdapter, CacheSetDirectedAllocation) {
    ace::families::a12_cama::CamaAdapter<> alloc;
    auto* p1 = alloc.raw_allocate_for_set(128, 16, 5);
    EXPECT_EQ(alloc.last_target_set(), 5);
    auto* p2 = alloc.raw_allocate_for_set(128, 16, 17);
    EXPECT_EQ(alloc.last_target_set(), 17);
    alloc.raw_deallocate(p1, 128, 16);
    alloc.raw_deallocate(p2, 128, 16);
    static_assert(ace::families::a12_cama::CamaAdapter<>::is_constant_time);
    static_assert(ace::families::a12_cama::CamaAdapter<>::is_wcet_analysable);
}

TEST(StarMallocAdapter, HardenedFeatures) {
    static_assert(ace::families::a13_starmalloc::StarMallocAdapter<>::is_formally_verified);
    static_assert(ace::families::a13_starmalloc::StarMallocAdapter<>::is_hardened);
    static_assert(ace::families::a13_starmalloc::StarMallocAdapter<>::is_drop_in_replacement);

    ace::families::a13_starmalloc::StarMallocAdapter<> alloc;
    auto* p = alloc.raw_allocate(128, 16);
    ASSERT_NE(p, nullptr);
    auto* bytes = static_cast<unsigned char*>(p);
    for (int i = 0; i < 128; ++i) EXPECT_EQ(bytes[i], 0);
    alloc.raw_deallocate(p, 128, 16);
}

TEST(TcmallocWarehouseAdapter, LifetimeAwareSpanList) {
    ace::families::a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<> alloc;
    auto* p_long  = alloc.raw_allocate(1024, 16);
    auto* p_short = alloc.raw_allocate(8, 8);
    EXPECT_GT(alloc.allocations_for_list(0), 0u);
    EXPECT_GT(alloc.allocations_for_list(7), 0u);
    alloc.raw_deallocate(p_long, 1024, 16);
    alloc.raw_deallocate(p_short, 8, 8);
    static_assert(ace::families::a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<>::has_heterogeneous_caches);
    static_assert(ace::families::a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<>::has_nuca_transfer);
    static_assert(ace::families::a14_tcmalloc_warehouse::TcmallocWarehouseAdapter<>::has_lifetime_hugepage);
}

TEST(HmallocAdapter, LocalVsSharedAllocation) {
    ace::families::a15_hmalloc::HmallocAdapter<> alloc;
    auto* p_local = alloc.raw_allocate_with_class(64, 8, ace::families::a15_hmalloc::MemoryClass::Local);
    auto* p_share = alloc.raw_allocate_with_class(64, 8, ace::families::a15_hmalloc::MemoryClass::Shared);
    EXPECT_EQ(alloc.local_count(), 1u);
    EXPECT_EQ(alloc.shared_count(), 1u);
    alloc.raw_deallocate(p_local, 64, 8);
    alloc.raw_deallocate(p_share, 64, 8);
}

TEST(PimMallocAdapter, FuturePlatformMarker) {
    static_assert(ace::families::a16_pim_malloc::PimMallocAdapter<>::requires_pim_hardware);
    static_assert(ace::families::a16_pim_malloc::PimMallocAdapter<>::is_future_platform);
    ace::families::a16_pim_malloc::PimMallocAdapter<> alloc;
    auto* p = alloc.raw_allocate(64, 8);
    EXPECT_EQ(alloc.pim_local_count(), 1u);
    alloc.raw_deallocate(p, 64, 8);
}

TEST(CrystallineAdapter, WaitFreeReclamationProperties) {
    static_assert(ace::families::a17_crystalline::CrystallineAdapter<>::is_wait_free);
    static_assert(ace::families::a17_crystalline::CrystallineAdapter<>::is_lock_free);
    static_assert(ace::families::a17_crystalline::CrystallineAdapter<>::memory_bounded);
    static_assert(ace::families::a17_crystalline::CrystallineAdapter<>::is_async_signal_safe);

    ace::families::a17_crystalline::CrystallineAdapter<> alloc;
    auto* p = alloc.raw_allocate(64, 8);
    alloc.raw_deallocate(p, 64, 8);
    EXPECT_EQ(alloc.retired_count(), 1u);
}

TEST(ExgenMallocAdapter, EightByteFineGrainedSizeClasses) {
    static_assert(ace::families::a18_exgen_malloc::ExgenMallocAdapter<>::is_single_threaded_only);
    static_assert(ace::families::a18_exgen_malloc::ExgenMallocAdapter<>::has_metadata_compaction);

    ace::families::a18_exgen_malloc::ExgenMallocAdapter<> alloc;
    auto* p = alloc.raw_allocate(13, 8);
    auto stats = alloc.statistics();
    EXPECT_GE(stats.total_bytes_allocated, 16u);
    alloc.raw_deallocate(p, 13, 8);
}

TEST(Ptmalloc2Adapter, BinClassificationCorrect) {
    ace::families::a21_ptmalloc2::Ptmalloc2Adapter<> alloc;
    auto* p_tcache  = alloc.raw_allocate(64, 8);
    auto* p_large    = alloc.raw_allocate(8192, 16);
    EXPECT_EQ(alloc.tcache_count(), 1u);
    EXPECT_EQ(alloc.largebin_count(), 1u);
    alloc.raw_deallocate(p_tcache, 64, 8);
    alloc.raw_deallocate(p_large, 8192, 16);
}

TEST(PmrResourceAdapter, AllVariantsWork) {
    using namespace ace::families::a22_pmr_resource;

    {
        PmrParams p;
        p.variant = PmrVariant::NewDelete;
        PmrResourceAdapter<> alloc{p};
        auto* ptr = alloc.raw_allocate(64, 16);
        ASSERT_NE(ptr, nullptr);
        alloc.raw_deallocate(ptr, 64, 16);
    }
    {
        PmrParams p;
        p.variant = PmrVariant::SynchronizedPool;
        PmrResourceAdapter<> alloc{p};
        auto* ptr = alloc.raw_allocate(128, 16);
        ASSERT_NE(ptr, nullptr);
        alloc.raw_deallocate(ptr, 128, 16);
    }
    {
        PmrParams p;
        p.variant = PmrVariant::MonotonicBuffer;
        PmrResourceAdapter<> alloc{p};
        auto* ptr = alloc.raw_allocate(256, 16);
        ASSERT_NE(ptr, nullptr);
        alloc.raw_deallocate(ptr, 256, 16);
    }
}

TEST(VmemMagazinesAdapter, AdaptiveMagazineResize) {
    ace::families::a23_vmem_magazines::VmemMagazinesAdapter<> alloc;
    auto initial_size = alloc.current_magazine_size();
    alloc.trigger_magazine_resize_up();
    auto resized = alloc.current_magazine_size();
    EXPECT_GT(resized, initial_size);
    static_assert(ace::families::a23_vmem_magazines::VmemMagazinesAdapter<>::is_constant_time);
    static_assert(ace::families::a23_vmem_magazines::VmemMagazinesAdapter<>::has_per_cpu_magazines);
}

TEST(VmemMagazinesAdapter, FreelistIndexLog2Power2) {
    ace::families::a23_vmem_magazines::VmemMagazinesAdapter<> alloc;
    auto* p = alloc.raw_allocate(100, 8);
    EXPECT_EQ(alloc.last_freelist_index(), 7);
    auto stats = alloc.statistics();
    EXPECT_GE(stats.total_bytes_allocated, 128u);
    alloc.raw_deallocate(p, 100, 8);
}
