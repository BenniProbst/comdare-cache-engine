// V41 #40 (Doku 24 §6.2, Doku 14 §3.3) — Tier→Organ-REKONSTRUKTIONS-Beleg.
// Belegt, dass jeder bereits sezierte monolithische Tier-Wrapper exakt aus seiner äquivalenten
// Organ-Komposition wiederherstellbar ist (Tier ≡ Organ-Komposition ≡ std::map). Das ist die Brücke
// für die Umstufung (Doku 24 §6.2): erst wenn die Äquivalenz belegt ist, darf der monolithische Wrapper
// aus der Achse entfernt + als Gattungs-Konfigurator (Composition über Organe) rekonstruiert werden.
// key-type-sicher: Vergleich INNERHALB der schmalen Tier-Breite (uint8 -> 200/255, uint16 -> 1000/1000),
// sonst kollidieren Keys beim Cast (organ=uint64 vs tier=uint8/uint16).

#include <gtest/gtest.h>

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_registry.hpp>          // Tier-Wrapper-Typen
#include <topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp>        // Organ-Pendants
#include "support/std_map_equivalence_harness.hpp"

#include <map>
#include <vector>

namespace ce_03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ce_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
namespace ts     = ::comdare::cache_engine::test_support;

// --- Jeder bereits sezierte Tier-Wrapper == std::map (vertikal, in seiner eigenen Key-Breite) -----------
TEST(Axis03aTierOrgan, EachDissectedTierMatchesStdMap) {
    ts::verify_matches_std_map<ce_03a::Array256SearchAlgo>(200u, 255u);          // uint8
    ts::verify_matches_std_map<ce_03a::VectorU8U8SearchAlgo>(200u, 255u);        // uint8
    ts::verify_matches_std_map<ce_03a::VectorU16U16SearchAlgo>(1000u, 1000u);    // uint16
    ts::verify_matches_std_map<ce_03a::Array65535SearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::KArySearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::EytzingerSearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::LinearScanSearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::InterpolationSearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::BinarySearchTreeSearchAlgo>(1000u, 1000u);
    SUCCEED();
}

// --- uint8-Tiere ≡ Organ-Komposition (REKONSTRUKTION belegt, key_mod=200) --------------------------------
TEST(Axis03aTierOrgan, Uint8TiersReconstructibleFromOrgan) {
    ts::verify_variants_equivalent<ce_cmp::LinearScanOrgan, ce_03a::Array256SearchAlgo>(200u, 255u);
    ts::verify_variants_equivalent<ce_cmp::LinearScanOrgan, ce_03a::VectorU8U8SearchAlgo>(200u, 255u);
    SUCCEED();  // dense/sparse uint8-Tier exakt aus LinearScan-Organ-Komposition wiederherstellbar
}

// --- uint16-Tiere ≡ Organ-Komposition (REKONSTRUKTION belegt, key_mod=1000) ------------------------------
TEST(Axis03aTierOrgan, Uint16TiersReconstructibleFromOrgan) {
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::VectorU16U16SearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::Array65535SearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::KArySearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::EytzingerSearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::LinearScanOrgan,   ce_03a::LinearScanSearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::InterpolationOrgan, ce_03a::InterpolationSearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::BstTreeOrgan,      ce_03a::BinarySearchTreeSearchAlgo>(1000u, 1000u);
    SUCCEED();  // jeder uint16-Tier exakt aus seiner Organ-Komposition wiederherstellbar
}

// --- Umstufung-A (#41): sezierte CE-native Strukturen ≡ Organ-Komposition (REKONSTRUKTION, key_mod=1000) --
// Hash (UNGEORDNET, open-addressing Fibonacci, Knuth TAOCP 3 §6.4): eigene HashBucketPool-Familie.
// Doppel-Beleg: vertikal (Organ ≡ std::map) + horizontal (Organ ≡ Monolith) ⇒ transitiv Organ ≡ std::map.
TEST(Axis03aTierOrgan, Uint16HashReconstructibleFromOrgan) {
    ts::verify_matches_std_map<ce_cmp::HashSearchOrgan>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::HashSearchOrgan, ce_03a::HashSearchAlgo>(1000u, 1000u);
    SUCCEED();  // HashSearchAlgo exakt aus ComposedHashSearch<HashProbeTraversalOrgan, HashBucketPoolStore> rekonstruierbar
}

// SkipList (GEORDNET probabilistisch, Pugh CACM 1990): eigene SkipListNodePool-Familie (RNG im Store, Seed 0xC0FFEEu).
TEST(Axis03aTierOrgan, Uint16SkipListReconstructibleFromOrgan) {
    ts::verify_matches_std_map<ce_cmp::SkipListOrgan>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SkipListOrgan, ce_03a::SkipListSearchAlgo>(1000u, 1000u);
    SUCCEED();  // SkipListSearchAlgo exakt aus ComposedSkipListSearch<SkipListTraversalOrgan, SkipListNodePoolStore> rekonstruierbar
}

// B-Baum (GEORDNET balanciert, Mehrwege t=4, block-orientiert, Bayer/McCreight / CLRS Kap.18): eigene BTreeNodePool-Familie.
TEST(Axis03aTierOrgan, Uint16BTreeReconstructibleFromOrgan) {
    ts::verify_matches_std_map<ce_cmp::BTreeSearchOrgan>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::BTreeSearchOrgan, ce_03a::BTreeSearchAlgo>(1000u, 1000u);
    SUCCEED();  // BTreeSearchAlgo exakt aus ComposedBTreeSearch<BTreeTraversalOrgan, BTreeNodePoolStore> rekonstruierbar
}

// --- Adversarialer B-Baum-Delete-Stresstest (#41 Inc3) -----------------------------------------------------
// Der B-Baum-Delete (11 von Hand auf Pool-Getter/Setter portierte CLRS-Helfer: borrow_from_prev/next, merge,
// remove_from_nonleaf, fill, Wurzel-Schrumpf) ist das riskanteste Stueck der Sezierung. Der Harness-600-Op-
// Stream uebt borrow/merge nur begrenzt. Dieser Test erzwingt sie gezielt: grosser Baum (mehrstufig bei t=4),
// dann drei delete-lastige Phasen (jede-2., aufsteigend, absteigend) mit voller std::map-Kreuzpruefung nach
// jeder Loeschung — bricht eine Borrow/Merge/Root-Shrink-Transformation, divergiert lookup/occupied_count sofort.
namespace {
void btree_cross_check(ce_cmp::BTreeSearchOrgan const& organ, std::map<std::uint64_t, std::uint64_t> const& ref,
                       std::uint64_t query_max) {
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (std::uint64_t q = 0; q <= query_max; ++q) {
        auto const o  = organ.lookup(q);
        auto const it = ref.find(q);
        if (it == ref.end()) { ASSERT_FALSE(o.has_value()) << "phantom key " << q; }
        else { ASSERT_TRUE(o.has_value()) << "lost key " << q; ASSERT_EQ(*o, it->second) << "value mismatch key " << q; }
    }
}
}  // namespace

TEST(Axis03aTierOrgan, BTreeDeleteStressMatchesStdMap) {
    constexpr std::uint64_t N = 400;   // mehrstufiger Baum bei t=4 (>= 3 Ebenen)
    ce_cmp::BTreeSearchOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;
    for (std::uint64_t k = 0; k < N; ++k) { organ.insert(k, k * 7 + 1); ref[k] = k * 7 + 1; }
    btree_cross_check(organ, ref, N);

    // Phase 1: jeden 2. Schluessel loeschen (erzwingt verstreute borrow/merge).
    for (std::uint64_t k = 0; k < N; k += 2) { organ.erase(k); ref.erase(k); }
    btree_cross_check(organ, ref, N);

    // Phase 2: verbleibende aufsteigend loeschen (erzwingt wiederholtes Links-Borrow/Merge + Root-Shrink).
    std::vector<std::uint64_t> remaining;
    for (auto const& kv : ref) remaining.push_back(kv.first);
    for (std::uint64_t k : remaining) {
        if (k % 4 == 1) { organ.erase(k); ref.erase(k); }
    }
    btree_cross_check(organ, ref, N);

    // Phase 3: Rest absteigend leeren (erzwingt Rechts-Borrow/Merge bis leerer Baum).
    remaining.clear();
    for (auto const& kv : ref) remaining.push_back(kv.first);
    for (auto it = remaining.rbegin(); it != remaining.rend(); ++it) { organ.erase(*it); ref.erase(*it); }
    btree_cross_check(organ, ref, N);
    ASSERT_EQ(organ.occupied_count(), 0u);

    // Re-Insert nach vollstaendiger Leerung (Free-List-Recycling + Wurzel-Neuaufbau).
    for (std::uint64_t k = 0; k < 50; ++k) { organ.insert(k, k + 100); ref[k] = k + 100; }
    btree_cross_check(organ, ref, N);
}
