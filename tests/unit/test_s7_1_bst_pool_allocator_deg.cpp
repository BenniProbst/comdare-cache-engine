// S7-1 (#261/#234) -- BST-Pool allocator parameter + T6 DEG hook.

#include "builder/measurement_snapshot.hpp"

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp> // IRollbackableTier (Zwei-Phasen-CoW)
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp> // detail::two_phase_measure
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
namespace comp = ::comdare::cache_engine::compositions;
namespace lk   = ::comdare::cache_engine::lookup;
namespace lkc  = ::comdare::cache_engine::lookup::composable;
namespace shp  = ::comdare::cache_engine::nodes::axis_bst_shape;
namespace alc  = ::comdare::cache_engine::alloc;
namespace acd  = ::comdare::cache_engine::builder::anatomy_commands::detail;

namespace {

using U64          = std::uint64_t;
using DefaultShape = shp::BstPtrSizeT;
using DefaultStore = lkc::TreeNodePoolStore<DefaultShape>;
using DefaultOrgan = lkc::BstTreeOrgan;
using BstHull      = lkc::ObservableComposedContainer<DefaultOrgan>;

static_assert(lkc::TreeNodePool<DefaultStore>);
// Phase 0.3 (Hebel B): allocator_type ist jetzt die axis_06-Strategie (Default ExgenAllocator, real=std bei
// disabled -> verhaltens-neutral) statt std::allocator — der Pool allokiert REAL ueber die Allocator-Achse.
static_assert(alc::concepts::AllocatorStrategy<typename DefaultStore::allocator_type>);
static_assert(std::is_same_v<typename DefaultStore::allocator_type, alc::ExgenAllocator>);
static_assert(requires(DefaultOrgan const& organ) { organ.store_allocator_statistics(); });
static_assert(requires(BstHull const& hull) { hull.store_allocator_statistics(); });

template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
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
    static constexpr std::string_view paper_id = "S7-1 BST pool allocator DEG";
    static constexpr std::string_view name     = "S71BstPoolComposition";
};

[[nodiscard]] U64 value_for(U64 key) noexcept { return key ^ 0x9E3779B97F4A7C15ull; }

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

} // namespace

TEST(S71BstPoolAllocatorDeg, DirectPoolStatsAreReal) {
    BstHull                hull;
    std::vector<U64> const keys{40u, 12u, 90u, 7u, 25u, 70u, 110u, 1u};

    for (U64 const key : keys) { EXPECT_TRUE(hull.insert(key, value_for(key))); }

    // Phase 0.3: store_allocator_statistics() liefert jetzt die axis_06-Strategie-Statistik (AllocationStatistics,
    // rich 5-Feld) statt der Store-eigenen Vektor-Kapazitaets-Zaehlung. Der Pool hat real ueber die Allocator-Achse
    // allokiert -> allocation_count/total_bytes_allocated > 0; total_bytes_in_use > 0 (Free-List -> nie vector-free).
    auto const stats = hull.store_allocator_statistics();
    EXPECT_GT(stats.allocation_count, 0u);
    EXPECT_GT(stats.total_bytes_allocated, 0u);
    EXPECT_GT(stats.total_bytes_in_use, 0u);
    // Exakter Belegungs-Invariant (statt des entfallenen live_nodes-Zaehlers): occupied_count == eingefuegte Keys.
    EXPECT_EQ(hull.occupied_count(), keys.size());
}

TEST(S71BstPoolAllocatorDeg, AbiObserverRoutesPoolAllocatorStatsToT6) {
    using C       = PoolFlipComposition<lk::BinarySearchTreeSearchAlgo>;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    auto  tier = std::make_unique<Adapter>();
    auto* drv  = static_cast<an::IDriveableTier*>(tier.get());
    auto* obs  = dynamic_cast<an::IObservableTier*>(drv);
    ASSERT_NE(obs, nullptr);

    std::vector<U64> const keys{32u, 16u, 48u, 8u, 24u, 40u, 56u, 4u, 12u};
    for (U64 const key : keys) { ASSERT_TRUE(drv->tier_insert(key, value_for(key))); }

    an::ComdareTierObserverSnapshot snap{};
    obs->tier_observe(&snap);

    std::cout << "S7-1 T6-DEG alloc_cnt=" << snap.axis_stats[6][2] << " bytes_alloc=" << snap.axis_stats[6][0]
              << " bytes_in_use=" << snap.axis_stats[6][1] << '\n';

    EXPECT_GT(snap.axis_stats[6][0], 0u);
    EXPECT_GT(snap.axis_stats[6][1], 0u);
    EXPECT_GT(snap.axis_stats[6][2], 0u);
}

// Phase 0.3 KERN-Regression (Memento/Option A): beweist, dass der strategie-getriebene Pool-Store COW-safe misst.
// Der 0.3a-Bug war exakt die Zwei-Phasen-Doppelzaehlung (COW-Vollkopie alloziert erneut ueber die Strategie ->
// T6 pollutet). Fahre dieselbe Sequenz ein- vs. zwei-phasig (save->warmup->rollback->measure je Op) und fordere
// COUNTER-CLEAN (2-phasig <= 1-phasig). Nur insert-Ops -> Free-List -> nie vector-free -> allocated==in_use.
TEST(S71BstPoolAllocatorDeg, TwoPhaseCowT6IsPollutionFree) {
    using C       = PoolFlipComposition<lk::BinarySearchTreeSearchAlgo>;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;
    // Der axis_06-getriebene Store bleibt CoW-fähig (copy-konstruierbar/assignable + Observer-restore_statistics).
    static_assert(Adapter::tier_memento_is_copy_on_write(),
                  "Phase 0.3: der axis_06-getriebene TreeNodePoolStore muss den lazy CoW-Memento-Pfad tragen");

    auto seq = [](an::IObservableTier& t, an::IRollbackableTier* rb) {
        t.tier_clear();
        for (U64 k = 1; k <= 40u; ++k) (void)t.tier_insert(k, k * 3u); // LOAD (ungemessen)
        for (U64 i = 0; i < 80u; ++i) {
            auto op = [&]() -> std::int64_t {
                (void)t.tier_insert(1000u + i, i);
                return 0;
            };
            if (rb != nullptr)
                (void)acd::two_phase_measure(rb, op);
            else
                (void)op();
        }
    };

    Adapter one_phase, two_phase;
    auto*   t1 = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&one_phase));
    auto*   t2 = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&two_phase));
    auto*   rb = dynamic_cast<an::IRollbackableTier*>(static_cast<an::IAnatomyBase*>(&two_phase));
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);
    ASSERT_NE(rb, nullptr);

    seq(*t1, nullptr);
    seq(*t2, rb);
    EXPECT_EQ(t1->tier_size(), t2->tier_size()); // beide Laeufe enden im selben Zustand

    an::ComdareTierObserverSnapshot s1{}, s2{};
    t1->tier_observe(&s1);
    t2->tier_observe(&s2);

    // Beide Laeufe messen REAL ueber die axis_06-Strategie (T6 > 0).
    EXPECT_GT(s1.axis_stats[6][0], 0u); // total_bytes_allocated
    EXPECT_GT(s2.axis_stats[6][0], 0u);
    EXPECT_GT(s1.axis_stats[6][2], 0u); // allocation_count
    EXPECT_GT(s2.axis_stats[6][2], 0u);
    // COUNTER-CLEAN — der eigentliche Beweis des Mementos: die save/rollback-Vollkopien werden per
    // restore_statistics neutralisiert -> keine Doppelzaehlung -> 2-phasig <= 1-phasig. (Ohne das Memento
    // waere s2 > s1, wie im revertierten 0.3a-Bug.)
    EXPECT_LE(s2.axis_stats[6][0], s1.axis_stats[6][0]);
    EXPECT_LE(s2.axis_stats[6][2], s1.axis_stats[6][2]);
    // ECHTE Allocator-Semantik: total_bytes_allocated (kumulativ, inkl. freigegebener vector-Wachstums-Puffer)
    // >= total_bytes_in_use (aktuell live) — der real messbare Unterschied zur alten allocator-unabhaengigen
    // Zaehlung; total_bytes_in_use > 0 (Knoten gehalten); keine Fehl-Allokation.
    EXPECT_GE(s1.axis_stats[6][0], s1.axis_stats[6][1]);
    EXPECT_GE(s2.axis_stats[6][0], s2.axis_stats[6][1]);
    EXPECT_GT(s1.axis_stats[6][1], 0u);
    EXPECT_GT(s2.axis_stats[6][1], 0u);
    EXPECT_EQ(s1.axis_stats[6][4], 0u);
    EXPECT_EQ(s2.axis_stats[6][4], 0u);
}

TEST(S71BstPoolAllocatorDeg, DefaultStoreRoundtripIsUnchanged) {
    DefaultOrgan           organ;
    std::vector<U64> const keys{5u, 2u, 8u, 1u, 3u, 7u, 9u};

    for (U64 const key : keys) { organ.insert(key, value_for(key)); }

    EXPECT_EQ(organ.occupied_count(), keys.size());
    for (U64 const key : keys) {
        auto const found = organ.lookup(key);
        ASSERT_TRUE(found.has_value()) << "key=" << key;
        EXPECT_EQ(*found, value_for(key));
    }
    EXPECT_FALSE(organ.lookup(404u).has_value());

    organ.insert(3u, 333u);
    auto const updated = organ.lookup(3u);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(*updated, 333u);
}

TEST(S71BstPoolAllocatorDeg, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 4);
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 5u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string>                     ids{"neutrality_guard"};
    std::vector<std::string>                     workloads{"ap9"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}
