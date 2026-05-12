// Tests fuer Concept-Wurzeln der Allokator-Bibliothek (Phase 6.2.E)
// IAllocationStrategy + CacheEngineAllocator + CacheEnginePmrResource +
// LockingStrategy.

#include <cache_engine/allocators/concepts/i_allocation_strategy.hpp>
#include <cache_engine/allocators/concepts/allocator_concept.hpp>
#include <cache_engine/allocators/concepts/pmr_resource_concept.hpp>
#include <cache_engine/allocators/concepts/locking_concept.hpp>
#include <cache_engine/allocators/locking/shared_mutex_lock.hpp>
#include <cache_engine/allocators/locking/cache_page_aware_lock.hpp>
#include <cache_engine/allocators/families/a01_hoard/hoard_adapter.hpp>
#include <cache_engine/allocators/families/a04_mimalloc/mimalloc_adapter.hpp>
#include <cache_engine/allocators/families/a06_tcmalloc/tcmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a07_snmalloc/snmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a20_dlmalloc/dlmalloc_adapter.hpp>
#include <cache_engine/allocators/families/a19_buddy/buddy_adapter.hpp>
#include <cache_engine/allocators/allocator_manager.hpp>

#include <gtest/gtest.h>

#include <deque>
#include <list>
#include <map>
#include <memory_resource>
#include <set>
#include <unordered_map>
#include <vector>

namespace ace = comdare::cache_engine::allocator;

// ─────────────────────────────────────────────────────────────────────────────
// Concept-Conformance
// ─────────────────────────────────────────────────────────────────────────────

TEST(AllocatorConcepts, AllFamilyAdaptersSatisfyConcept) {
    using namespace ace::families;
    static_assert(ace::IAllocationStrategy<a01_hoard::HoardAdapter<>>);
    static_assert(ace::IAllocationStrategy<a04_mimalloc::MimallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a06_tcmalloc::TcmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a07_snmalloc::SnmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a20_dlmalloc::DlmallocAdapter<>>);
    static_assert(ace::IAllocationStrategy<a19_buddy::BuddyAdapter<>>);
    SUCCEED();
}

TEST(AllocatorConcepts, LockingStrategyConformance) {
    static_assert(ace::LockingStrategy<ace::locking::SharedMutexLock>);
    static_assert(ace::LockingStrategy<ace::locking::CachePageAwareLock>);
    static_assert(ace::CachePageAwareLockingStrategy<ace::locking::CachePageAwareLock>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// HoardAdapter Basic Allocation
// ─────────────────────────────────────────────────────────────────────────────

TEST(HoardAdapter, AllocAndDealloc) {
    ace::families::a01_hoard::HoardAdapter<> allocator;
    auto* p = allocator.raw_allocate(128, 16);
    ASSERT_NE(p, nullptr);
    allocator.raw_deallocate(p, 128, 16);
    auto stats = allocator.statistics();
    EXPECT_EQ(stats.allocation_count, 1u);
    EXPECT_EQ(stats.deallocation_count, 1u);
}

TEST(HoardAdapter, ParamsAccessible) {
    ace::families::a01_hoard::HoardAdapter<> allocator;
    EXPECT_EQ(allocator.params().superblock_bytes, 8u * 1024u);
    EXPECT_DOUBLE_EQ(allocator.params().empty_fraction, 0.25);
}

// ─────────────────────────────────────────────────────────────────────────────
// MimallocAdapter mit Deferred-Free
// ─────────────────────────────────────────────────────────────────────────────

TEST(MimallocAdapter, AllocateAndStatistics) {
    ace::families::a04_mimalloc::MimallocAdapter<> allocator;
    auto* p1 = allocator.raw_allocate(64, 16);
    auto* p2 = allocator.raw_allocate(256, 32);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    auto stats = allocator.statistics();
    EXPECT_EQ(stats.allocation_count, 2u);
    EXPECT_GE(stats.total_bytes_allocated, 64u + 256u);
    allocator.raw_deallocate(p1, 64, 16);
    allocator.raw_deallocate(p2, 256, 32);
}

TEST(MimallocAdapter, DeferredFreeCallback) {
    ace::families::a04_mimalloc::MimallocAdapter<> allocator;
    int callback_count = 0;
    allocator.set_deferred_free(
        [](void* data) { ++(*static_cast<int*>(data)); },
        &callback_count);
    allocator.trigger_deferred_free();
    allocator.trigger_deferred_free();
    EXPECT_EQ(callback_count, 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// BuddyAdapter XOR-Buddy
// ─────────────────────────────────────────────────────────────────────────────

TEST(BuddyAdapter, XorBuddyAddressCorrect) {
    auto buddy = ace::families::a19_buddy::BuddyAdapter<>::buddy_address(0x1000, 12);
    EXPECT_EQ(buddy, 0x2000u);

    auto buddy2 = ace::families::a19_buddy::BuddyAdapter<>::buddy_address(0x10000, 12);
    EXPECT_EQ(buddy2, 0x11000u);
}

TEST(BuddyAdapter, AllocationRoundsToPowerOf2) {
    ace::families::a19_buddy::BuddyAdapter<> allocator;
    auto* p = allocator.raw_allocate(100, 16);   // → rounds to 128 (2^7)
    ASSERT_NE(p, nullptr);
    auto stats = allocator.statistics();
    EXPECT_GE(stats.total_bytes_allocated, 128u);
    EXPECT_GT(stats.internal_fragmentation, 0.0);
    allocator.raw_deallocate(p, 100, 16);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheEngineAllocator<T> mit std-Container
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheEngineAllocator, WorksWithStdVector) {
    ace::families::a01_hoard::HoardAdapter<> strategy;
    ace::CacheEngineAllocator<int, decltype(strategy)> alloc{&strategy};
    std::vector<int, decltype(alloc)> vec(alloc);
    for (int i = 0; i < 100; ++i) vec.push_back(i);
    EXPECT_EQ(vec.size(), 100u);
    EXPECT_EQ(vec[42], 42);
}

TEST(CacheEngineAllocator, WorksWithStdList) {
    ace::families::a04_mimalloc::MimallocAdapter<> strategy;
    ace::CacheEngineAllocator<int, decltype(strategy)> alloc{&strategy};
    std::list<int, decltype(alloc)> lst(alloc);
    for (int i = 0; i < 10; ++i) lst.push_back(i * 7);
    EXPECT_EQ(lst.size(), 10u);
    EXPECT_EQ(lst.back(), 63);
}

TEST(CacheEngineAllocator, WorksWithStdDeque) {
    ace::families::a06_tcmalloc::TcmallocAdapter<> strategy;
    ace::CacheEngineAllocator<int, decltype(strategy)> alloc{&strategy};
    std::deque<int, decltype(alloc)> dq(alloc);
    for (int i = 0; i < 50; ++i) dq.push_back(i);
    EXPECT_EQ(dq.size(), 50u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheEnginePmrResource mit std::pmr-Container (REV 7 Pflicht!)
// ─────────────────────────────────────────────────────────────────────────────

TEST(PmrResource, WorksWithPmrVector) {
    ace::families::a01_hoard::HoardAdapter<> strategy;
    ace::CacheEnginePmrResource<decltype(strategy)> resource{&strategy};
    std::pmr::vector<int> vec{&resource};
    for (int i = 0; i < 1000; ++i) vec.push_back(i);
    EXPECT_EQ(vec.size(), 1000u);
    EXPECT_EQ(vec[500], 500);
}

TEST(PmrResource, WorksWithPmrMap) {
    ace::families::a04_mimalloc::MimallocAdapter<> strategy;
    ace::CacheEnginePmrResource<decltype(strategy)> resource{&strategy};
    std::pmr::map<int, std::string> m{&resource};
    m[1] = "one";
    m[2] = "two";
    m[3] = "three";
    EXPECT_EQ(m[2], "two");
    EXPECT_EQ(m.size(), 3u);
}

TEST(PmrResource, WorksWithPmrUnorderedMap) {
    ace::families::a07_snmalloc::SnmallocAdapter<> strategy;
    ace::CacheEnginePmrResource<decltype(strategy)> resource{&strategy};
    std::pmr::unordered_map<int, int> m{&resource};
    for (int i = 0; i < 100; ++i) m[i] = i * 2;
    EXPECT_EQ(m[50], 100);
    EXPECT_EQ(m.size(), 100u);
}

TEST(PmrResource, WorksWithPmrSet) {
    ace::families::a20_dlmalloc::DlmallocAdapter<> strategy;
    ace::CacheEnginePmrResource<decltype(strategy)> resource{&strategy};
    std::pmr::set<int> s{&resource};
    for (int i = 0; i < 50; ++i) s.insert(i);
    EXPECT_EQ(s.size(), 50u);
}

TEST(PmrResource, EqualityCorrect) {
    ace::families::a01_hoard::HoardAdapter<> strategy_a;
    ace::families::a01_hoard::HoardAdapter<> strategy_b;
    ace::CacheEnginePmrResource<decltype(strategy_a)> resource_a{&strategy_a};
    ace::CacheEnginePmrResource<decltype(strategy_b)> resource_b{&strategy_b};
    ace::CacheEnginePmrResource<decltype(strategy_a)> resource_a2{&strategy_a};

    EXPECT_TRUE(resource_a.is_equal(resource_a2));     // same strategy
    EXPECT_FALSE(resource_a.is_equal(resource_b));     // different strategy
}

// ─────────────────────────────────────────────────────────────────────────────
// PermutationFlags
// ─────────────────────────────────────────────────────────────────────────────

TEST(AllocatorPermutationFlags, PflichtPermutationsValid) {
    using namespace ace::permutations;
    EXPECT_TRUE(kHoardStyle.is_specified());
    EXPECT_TRUE(kMimallocStyle.is_specified());
    EXPECT_TRUE(kTcmallocModern.is_specified());
    EXPECT_TRUE(kPrtArtPool.is_specified());
    EXPECT_TRUE(kHardenedComdare.is_specified());
    EXPECT_TRUE(kLocklessComdare.is_specified());
    EXPECT_TRUE(kNumaComdare.is_specified());
    EXPECT_TRUE(kRealtimeComdare.is_specified());
    EXPECT_TRUE(kSingleThreadOptimum.is_specified());
}

TEST(AllocatorPermutationFlags, FlagsAggregation) {
    ace::AllocatorPermutationFlags flags{};
    EXPECT_FALSE(flags.is_specified());
    flags = ace::permutations::kMimallocStyle;
    EXPECT_TRUE(flags.is_specified());
    EXPECT_EQ(flags.aa1, ace::FreeListTopology::PageSharded3List_Mimalloc);
    EXPECT_EQ(flags.aa3, ace::ThreadLocality::TcachePerThread);
}

// ─────────────────────────────────────────────────────────────────────────────
// AllocatorManager - Bindeglied
// ─────────────────────────────────────────────────────────────────────────────

TEST(AllocatorManager, AggregatesStrategyAndFlags) {
    ace::families::a01_hoard::HoardAdapter<> strategy;
    ace::AllocatorManager mgr{std::move(strategy), ace::permutations::kHoardStyle};
    EXPECT_TRUE(mgr.flags().is_specified());

    auto* res = mgr.as_pmr();
    ASSERT_NE(res, nullptr);

    std::pmr::vector<int> vec{res};
    vec.push_back(42);
    EXPECT_EQ(vec[0], 42);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cache-Page-Aware Lock (Multi-Writer optional)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CachePageAwareLock, AcquireReleaseForSize) {
    ace::locking::CachePageAwareLock lock;
    lock.write_lock_for_size(4096);      // page 1
    lock.release_write_for_size(4096);

    lock.write_lock_for_size(8192);      // page 2
    lock.release_write_for_size(8192);
    SUCCEED();
}

TEST(CachePageAwareLock, MultiPageGuardWorks) {
    ace::locking::CachePageAwareLock lock;
    {
        ace::locking::CachePageAwareLock::MultiPageGuard guard{lock, 4096, 8192};
        // both pages now locked atomically (deadlock-vermeidend)
    }
    // released after guard out of scope
    SUCCEED();
}
