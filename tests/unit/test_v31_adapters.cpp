// V31.F (2026-05-14) — Smoke tests for all 22 V31-Adapter skeletons.
// Konstruieren+destruieren ohne crash + paper_id() liefert non-empty string.
// Echte Mess-Pipeline kommt mit V21.2 HW-E2E (User-Pending).

#include <gtest/gtest.h>

// SOTA adapters
#include "adapters/P01-ART/p01_art_adapter.hpp"
#include "adapters/P02-HOT/p02_hot_adapter.hpp"
#include "adapters/P03-Masstree/p03_masstree_adapter.hpp"
#include "adapters/P04-CoCo-trie/p04_coco_trie_adapter.hpp"
#include "adapters/P05-START/p05_start_adapter.hpp"
#include "adapters/P06-B2tree/p06_b2tree_adapter.hpp"
#include "adapters/P07-Wormhole/p07_wormhole_adapter.hpp"
#include "adapters/P10-SuRF/p10_surf_adapter.hpp"
#include "adapters/P20-BTreesAreBack/p20_leanstore_adapter.hpp"
#include "adapters/P25-Mahling/p25_mahling_adapter.hpp"
#include "adapters/P29-RCU/p29_rcu_adapter.hpp"
#include "adapters/P30-HazardPointers/p30_hazard_pointers_adapter.hpp"

// Allocator adapters
#include "adapters/A01-hoard/a01_hoard_adapter.hpp"
#include "adapters/A03-michael-lockfree/a03_michael_adapter.hpp"
#include "adapters/A04-mimalloc/a04_mimalloc_adapter.hpp"
#include "adapters/A05-jemalloc/a05_jemalloc_adapter.hpp"
#include "adapters/A06-tcmalloc/a06_tcmalloc_adapter.hpp"
#include "adapters/A07-snmalloc/a07_snmalloc_adapter.hpp"
#include "adapters/A08-scalloc/a08_scalloc_adapter.hpp"
#include "adapters/A10-rpmalloc/a10_rpmalloc_adapter.hpp"
#include "adapters/A11-lrmalloc/a11_lrmalloc_adapter.hpp"
#include "adapters/A20-dlmalloc/a20_dlmalloc_adapter.hpp"

#include <cstdint>
#include <cstring>
#include <string>

namespace {

template <typename Adapter>
void check_paper_id() {
    const char* id = Adapter::paper_id();
    ASSERT_NE(id, nullptr);
    EXPECT_GT(std::strlen(id), 0u);
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────
// SOTA-Adapter smoke tests
// ──────────────────────────────────────────────────────────────────────────
TEST(V31AdapterSota, P01_Art_Construct) {
    using A = comdare::adapter::p01_art::UnodbDbAdapter<>;
    A a;
    EXPECT_EQ(a.size(), 0u);
    check_paper_id<A>();
}

TEST(V31AdapterSota, P02_Hot_Construct) {
    using A = comdare::adapter::p02_hot::HotAdapter<>;
    A a;
    (void)a;
    check_paper_id<A>();
}

TEST(V31AdapterSota, P03_Masstree_Construct) {
    using A = comdare::adapter::p03_masstree::MasstreeAdapter<>;
    A a;
    (void)a;
    check_paper_id<A>();
}

TEST(V31AdapterSota, P04_CocoTrie_Construct) {
    comdare::adapter::p04_coco_trie::CocoTrieAdapter a;
    (void)a;
    check_paper_id<comdare::adapter::p04_coco_trie::CocoTrieAdapter>();
}

TEST(V31AdapterSota, P05_Start_Construct) {
    using A = comdare::adapter::p05_start::StartAdapter<>;
    A a;
    (void)a;
    check_paper_id<A>();
}

TEST(V31AdapterSota, P06_B2tree_Construct) {
    comdare::adapter::p06_b2tree::B2TreeAdapter a;
    (void)a;
    check_paper_id<comdare::adapter::p06_b2tree::B2TreeAdapter>();
}

TEST(V31AdapterSota, P07_Wormhole_Construct) {
    comdare::adapter::p07_wormhole::WormholeAdapter a;
    (void)a;
    check_paper_id<comdare::adapter::p07_wormhole::WormholeAdapter>();
}

TEST(V31AdapterSota, P10_Surf_Construct) {
    comdare::adapter::p10_surf::SurfAdapter a;
    (void)a;
    check_paper_id<comdare::adapter::p10_surf::SurfAdapter>();
}

TEST(V31AdapterSota, P20_LeanStore_Construct) {
    using A = comdare::adapter::p20_leanstore::LeanStoreAdapter<>;
    A a;
    (void)a;
    check_paper_id<A>();
}

TEST(V31AdapterSota, P25_Mahling_Construct) {
    comdare::adapter::p25_mahling::FillBufferProbe a;
    (void)a;
    check_paper_id<comdare::adapter::p25_mahling::FillBufferProbe>();
}

TEST(V31AdapterSota, P29_Rcu_Construct) {
    comdare::adapter::p29_rcu::LiburcuAdapter a;
    (void)a;
    check_paper_id<comdare::adapter::p29_rcu::LiburcuAdapter>();
}

TEST(V31AdapterSota, P30_HazardPointers_Construct) {
    comdare::adapter::p30_hazard_pointers::HazardPointerAdapter<int> a;
    (void)a;
    check_paper_id<comdare::adapter::p30_hazard_pointers::HazardPointerAdapter<int>>();
}

// ──────────────────────────────────────────────────────────────────────────
// Allokator-Adapter: Konstruieren + allocate/deallocate (Fallback std::malloc
// wenn ext-Repo nicht aktiv).
// ──────────────────────────────────────────────────────────────────────────
template <typename A>
void check_allocator_smoke() {
    A     a;
    void* p = a.allocate(64);
    ASSERT_NE(p, nullptr);
    std::memset(p, 0xAB, 64);
    a.deallocate(p);
    check_paper_id<A>();
}

TEST(V31AdapterAlloc, A01_Hoard) { check_allocator_smoke<comdare::adapter::a01_hoard::HoardAdapter>(); }
TEST(V31AdapterAlloc, A03_Michael) { check_allocator_smoke<comdare::adapter::a03_michael::MichaelAdapter>(); }
TEST(V31AdapterAlloc, A04_Mimalloc) {
    using A = comdare::adapter::a04_mimalloc::MimallocAdapter;
    A     a;
    void* p = a.allocate(64);
    ASSERT_NE(p, nullptr);
    a.deallocate(p);
    check_paper_id<A>();
}
TEST(V31AdapterAlloc, A05_Jemalloc) { check_allocator_smoke<comdare::adapter::a05_jemalloc::JemallocAdapter>(); }
TEST(V31AdapterAlloc, A06_Tcmalloc) { check_allocator_smoke<comdare::adapter::a06_tcmalloc::TcmallocAdapter>(); }
TEST(V31AdapterAlloc, A07_Snmalloc) { check_allocator_smoke<comdare::adapter::a07_snmalloc::SnmallocAdapter>(); }
TEST(V31AdapterAlloc, A08_Scalloc) { check_allocator_smoke<comdare::adapter::a08_scalloc::ScallocAdapter>(); }
TEST(V31AdapterAlloc, A10_Rpmalloc) { check_allocator_smoke<comdare::adapter::a10_rpmalloc::RpmallocAdapter>(); }
TEST(V31AdapterAlloc, A11_Lrmalloc) { check_allocator_smoke<comdare::adapter::a11_lrmalloc::LrmallocAdapter>(); }
TEST(V31AdapterAlloc, A20_Dlmalloc) { check_allocator_smoke<comdare::adapter::a20_dlmalloc::DlmallocAdapter>(); }

TEST(V31AdapterAlloc, A04_Mimalloc_OriginalActiveMatchesFlag) {
    constexpr bool active = comdare::adapter::a04_mimalloc::MimallocAdapter::original_active();
#if defined(COMDARE_HAVE_MIMALLOC)
    EXPECT_TRUE(active);
#else
    EXPECT_FALSE(active);
#endif
}
