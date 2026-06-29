// V5-I6-SUBSTANZ (#44) — ECHTE produktive MementoAxis auf den Such-Organen (nicht nur Concept/Pauschalkopie).
//
// Beweist (Re-Audit-Blocker 3): die komponierbaren Such-Organe ComposedSearch + ObservableComposedSearch
// erfüllen das MementoAxis-Concept (memento_aggregate.hpp) MIT echtem save_state/restore_state — der vom /goal
// geforderte „per-Achsen-Memento je stateful Achsen-Interface" statt der Adapter-Pauschalkopie. Round-Trip
// rollt Daten (über Traversal rekonstruiert) UND Observer-Stats exakt zurück.

#include "anatomy/memento_aggregate.hpp"
#include <axes/lookup/composable/observable_composed_search.hpp>

#include <gtest/gtest.h>

namespace cs = ::comdare::cache_engine::lookup::composable;
namespace an = ::comdare::cache_engine::anatomy;

using Organ    = cs::ComposedSearch<cs::SortedBinaryTraversal, cs::RawSlotStore>;
using ObsOrgan = cs::ObservableComposedSearch<cs::SortedBinaryTraversal, cs::RawSlotStore>;

// Das nackte Such-Organ ist eine MementoAxis + rollt seinen Daten-Zustand exakt zurück (via Traversal).
TEST(V5OrganMemento, ComposedSearchIsMementoAxisAndRoundTrips) {
    static_assert(an::MementoAxis<Organ>, "ComposedSearch MUSS MementoAxis sein (#44 Substanz)");
    Organ o;
    o.insert(5, 50);
    o.insert(3, 30);
    o.insert(7, 70);
    auto const saved = o.save_state(); // Warmup-Vor-Zustand (3 Einträge)
    o.insert(9, 90);
    (void)o.erase(3);       // Warmup-Mutation
    o.restore_state(saved); // rollback-all
    EXPECT_EQ(o.occupied_count(), 3u);
    EXPECT_EQ(o.lookup(3).value_or(0), 30u); // erase zurückgerollt
    EXPECT_EQ(o.lookup(5).value_or(0), 50u);
    EXPECT_EQ(o.lookup(7).value_or(0), 70u);
    EXPECT_FALSE(o.lookup(9).has_value()); // Warmup-insert zurückgerollt
}

// Der Observable-Wrapper ist eine MementoAxis + rollt Daten UND Observer-Stats zurück (Zwei-Phasen-Invariante).
TEST(V5OrganMemento, ObservableComposedSearchIsMementoAxis) {
    static_assert(an::MementoAxis<ObsOrgan>, "ObservableComposedSearch MUSS MementoAxis sein (#44 Substanz)");
    ObsOrgan o;
    (void)o.insert(1, 10);
    (void)o.insert(2, 20);
    (void)o.lookup(1);
    auto const saved = o.save_state();
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const stats_before = o.statistics();
#endif
    (void)o.insert(3, 30);
    (void)o.lookup(2);
    (void)o.lookup(99);                // Warmup: Daten + Stats mutiert
    o.restore_state(saved);            // rollback (KEIN Op danach vor dem Check)
    EXPECT_EQ(o.occupied_count(), 2u); // Daten zurückgerollt
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const stats_after = o.statistics();
    EXPECT_EQ(stats_after.total_insert_count, stats_before.total_insert_count); // Stats zurückgerollt
    EXPECT_EQ(stats_after.total_lookup_count, stats_before.total_lookup_count);
    EXPECT_EQ(stats_after.total_hit_count, stats_before.total_hit_count);
#endif
    EXPECT_EQ(o.lookup(1).value_or(0), 10u); // (dieser lookup zählt wieder — nach dem Check)
}
