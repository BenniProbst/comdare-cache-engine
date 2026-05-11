// test_three_layer_audit.cpp - 3-Schichten-Audit (Konzept/Strategie/Physisch)
// REV 5.1 INode-Korrektur + IRootNode statt IRootPage

#include "prt_art/concepts/i_node.hpp"
#include "prt_art/concepts/i_root_node.hpp"
#include "prt_art/concepts/i_fanout.hpp"
#include "prt_art/concepts/i_search_page.hpp"
#include "prt_art/concepts/i_search_page_structure.hpp"
#include "prt_art/concepts/i_search_page_structure_interpreter.hpp"
#include "prt_art/concepts/i_cache_page.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace pa = comdare::prt_art;

// -----------------------------------------------------------------------------
// SCHICHT 1 - KONZEPT
// -----------------------------------------------------------------------------
TEST(ThreeLayerAudit, INodeIsSlotMicroReference) {
    // REV 5.1 Korrektur: INode ist Slot-Eintrag, NICHT Container
    using IntNode = pa::INode<int, int>;
    // INode hat KEINE eigenstaendige Speicher-Adresse;
    // sie kennt nur ihre placement_page + slot_index.
    IntNode node{};
    EXPECT_EQ(node.placement_page(), nullptr);
    EXPECT_EQ(node.slot_index(), 0u);
    EXPECT_EQ(node.kind(), pa::NodeRefKind::ValueRef);  // Default
}

TEST(ThreeLayerAudit, IRootNodeReplacesIRootPage) {
    // REV 5.1: Wurzel ist KONZEPTIONELL (IRootNode), NICHT physisch (war: IRootPage)
    // Diese Datei kann nicht IRootPage einschliessen (existiert nicht mehr).
    using IntRoot = pa::IRootNode<int, int>;
    EXPECT_TRUE(std::is_abstract_v<IntRoot>);  // weil algorithm_signature pure-virtual
}

TEST(ThreeLayerAudit, IFanoutHasPagesNotNodes) {
    // REV 5.1 Korrektur: IFanout HAT ISearchPages (nicht INodes direkt)
    using IntFanout = pa::IFanout<int, int>;
    IntFanout fanout{};
    EXPECT_EQ(fanout.pages_count(), 0u);  // initial leer
    // INodes sind nur INDIREKT ueber ISearchPage erreichbar.
}

// -----------------------------------------------------------------------------
// SCHICHT 2 - STRATEGIE
// -----------------------------------------------------------------------------
TEST(ThreeLayerAudit, ISearchPageHasFanoutWidth15) {
    // Default-Width = Masstree W=15 (Member-Konstante)
    EXPECT_EQ(pa::kDefaultFanoutWidth, 15u);
}

TEST(ThreeLayerAudit, LayoutInvariantSetSupportsI4) {
    pa::LayoutInvariantSet inv{};
    inv.set(pa::LayoutInvariantKind::Invariant_I4);
    EXPECT_TRUE(inv.has(pa::LayoutInvariantKind::Invariant_I4));
    EXPECT_FALSE(inv.has(pa::LayoutInvariantKind::CacheLineAlign));
}

// -----------------------------------------------------------------------------
// SCHICHT 3 - PHYSISCH
// -----------------------------------------------------------------------------
TEST(ThreeLayerAudit, ICachePageIsPhysical) {
    using IntCachePage = pa::ICachePage<int, int>;
    EXPECT_TRUE(std::is_abstract_v<IntCachePage>);
}

TEST(ThreeLayerAudit, FragmentationKindKnowsThreeStates) {
    EXPECT_NE(static_cast<int>(pa::FragmentationKind::NotPresent),
              static_cast<int>(pa::FragmentationKind::WhollyContained));
    EXPECT_NE(static_cast<int>(pa::FragmentationKind::WhollyContained),
              static_cast<int>(pa::FragmentationKind::Fragmented));
}
