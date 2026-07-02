// V41 Roadmap-4 / Saeule 3 (Doku 24 §2.3) — Achsen-Vergleich gegen das vereinheitlichte std::map-Interface.
// Die KORREKTHEITS-/Interface-Dimension (latenzfrei, getrennt von der Tier-Wall-Clock): alle composable
// Such-Achsen-Varianten sind (vertikal) == std::map UND (horizontal) == untereinander — austauschbare
// Varianten EINER Achse, die sich nur im INNEN-Verhalten unterscheiden (geordnet vs. unsortiert vs. Baum).

#include <gtest/gtest.h>

#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/interpolation_traversal_organ.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/galloping_traversal_organ.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composed_tree_search.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_composed_store.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_pmr_resource.hpp>

#include "support/std_map_equivalence_harness.hpp"

namespace ce_cmp    = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
namespace ce_nodes  = ::comdare::cache_engine::nodes::axis_04_node_type;
namespace ce_layout = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace ce_alloc  = ::comdare::cache_engine::allocator::axis_06_allocator;
namespace ts        = ::comdare::cache_engine::test_support;

// Such-Achsen-Varianten ueber GEMEINSAMEM uint64-Key (alle teilen denselben Op-Stream).
using LinearFlat = ce_cmp::ComposedSearch<ce_cmp::LinearScanTraversal, ce_cmp::RawSlotStore>;
using SortedFlat = ce_cmp::ComposedSearch<ce_cmp::SortedBinaryTraversal, ce_cmp::RawSlotStore>;
using InterpFlat = ce_cmp::ComposedSearch<ce_cmp::InterpolationTraversalOrgan, ce_cmp::RawSlotStore>;
using GallopFlat = ce_cmp::ComposedSearch<ce_cmp::GallopingTraversalOrgan, ce_cmp::RawSlotStore>;
using BstTree    = ce_cmp::ComposedTreeSearch<ce_cmp::BSTTraversalOrgan, ce_cmp::TreeNodePoolStore<>>;

// --- (1) VERTIKAL: jede Flat-Variante einzeln == std::map -------------------------------------------------
TEST(Axis03aCrossVariant, FlatOrgansEquivalentToStdMap) {
    ts::verify_matches_std_map<LinearFlat>(100000u, 2000u);
    ts::verify_matches_std_map<SortedFlat>(100000u, 2000u);
    ts::verify_matches_std_map<InterpFlat>(100000u, 2000u);
    ts::verify_matches_std_map<GallopFlat>(100000u, 2000u);
    SUCCEED();
}

// --- (2) HORIZONTAL: alle 4 Flat-Varianten == untereinander (Kern-These Austauschbarkeit) -----------------
TEST(Axis03aCrossVariant, FlatOrgansEquivalentToEachOther) {
    ts::verify_variants_equivalent<LinearFlat, SortedFlat, InterpFlat, GallopFlat>(100000u, 2000u);
    SUCCEED();
}

// --- (3) Baum-Linie == std::map UND == Flat-Anker (andere Topologie, gleiche Semantik) --------------------
TEST(Axis03aCrossVariant, TreeOrganEquivalentToFlatAnchor) {
    ts::verify_matches_std_map<BstTree>(100000u, 2000u);
    ts::verify_variants_equivalent<SortedFlat, BstTree>(100000u, 2000u);
    SUCCEED();
}

// --- (4) Storage-Achsen-Tausch aendert das Resultat nicht (Mimalloc vs PMR; LinearScan + SortedBinary) ----
TEST(Axis03aCrossVariant, StorageSwapEquivalent) {
    using StoreMi  = ce_nodes::ComposedStore<ce_nodes::Node4NodeType, ce_layout::CacheLineAlignedMemoryLayout,
                                             ce_alloc::MimallocAllocator>;
    using StorePmr = ce_nodes::ComposedStore<ce_nodes::Node4NodeType, ce_layout::CacheLineAlignedMemoryLayout,
                                             ce_alloc::PmrResourceAllocator>;
    ts::verify_variants_equivalent<ce_cmp::ComposedSearch<ce_cmp::LinearScanTraversal, StoreMi>,
                                   ce_cmp::ComposedSearch<ce_cmp::LinearScanTraversal, StorePmr>,
                                   ce_cmp::ComposedSearch<ce_cmp::SortedBinaryTraversal, StoreMi>,
                                   ce_cmp::ComposedSearch<ce_cmp::SortedBinaryTraversal, StorePmr>>(100000u, 2000u);
    SUCCEED();
}

// --- (5) Eigenschafts-Tabelle: Innen-Verhalten ist HETEROGEN, das std::map-Resultat aber IDENTISCH --------
enum class OrderClass { Unordered, Ordered, Tree };
template <class Tr>
inline constexpr OrderClass kOrderClass = OrderClass::Unordered; // Default: LinearScan
template <>
inline constexpr OrderClass kOrderClass<ce_cmp::SortedBinaryTraversal> = OrderClass::Ordered;
template <>
inline constexpr OrderClass kOrderClass<ce_cmp::InterpolationTraversalOrgan> = OrderClass::Ordered;
template <>
inline constexpr OrderClass kOrderClass<ce_cmp::GallopingTraversalOrgan> = OrderClass::Ordered;
template <>
inline constexpr OrderClass kOrderClass<ce_cmp::BSTTraversalOrgan> = OrderClass::Tree;

TEST(Axis03aCrossVariant, OrderingPropertyTableHeterogeneousButResultsEqual) {
    // Die Tabelle ist HETEROGEN (>= 2 distinkte Innen-Verhalten-Klassen) ...
    static_assert(kOrderClass<ce_cmp::LinearScanTraversal> == OrderClass::Unordered);
    static_assert(kOrderClass<ce_cmp::SortedBinaryTraversal> == OrderClass::Ordered);
    static_assert(kOrderClass<ce_cmp::BSTTraversalOrgan> == OrderClass::Tree);
    static_assert(kOrderClass<ce_cmp::LinearScanTraversal> != kOrderClass<ce_cmp::SortedBinaryTraversal>);
    static_assert(kOrderClass<ce_cmp::SortedBinaryTraversal> != kOrderClass<ce_cmp::BSTTraversalOrgan>);
    // ... und DENNOCH liefern unsortiert (Linear) und Baum (BST) dasselbe std::map-Resultat:
    ts::verify_variants_equivalent<LinearFlat, BstTree>(100000u, 2000u);
    SUCCEED(); // Korrektheit ⊥ Innen-Verhalten — der Achsen-Vergleich auf der Interface-Dimension
}

// Hinweis (Doku 24 §2.4): Diese Suite traegt KEINE Latenz-/Wall-Clock-/Throughput-Felder. Die Frage
// "welche Variante SCHNELLER ist" gehoert zur Tier-Dimension (Roadmap-3 TierObserveTrace / V42), NICHT hierher.
