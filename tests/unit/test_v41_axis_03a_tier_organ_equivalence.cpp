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

// --- Umstufung-A (#41 Schritt B): OriginalXxx-Tiere ≡ Organ-Komposition (REKONSTRUKTION) ------------------
// Die OriginalXxx-Wrapper-Bodies sind triviale Re-Impls (ART=array256-dense, HOT/START/Wormhole/SuRF=
// sorted-vector+lower_bound) → seziert auf bestehende flache Organe. is_original/Habich bleibt am Wrapper;
// hier wird NUR das IST-Such-Verhalten als Organ-Komposition belegt (echte Trie-Anatomie = s4-Folge-Charge).
// Doppel-Beleg transitiv: Organ ≡ std::map (über identische Organ-Typen) + Organ ≡ OriginalXxx (hier).
TEST(Axis03aTierOrgan, Uint8OriginalTiersReconstructibleFromOrgan) {
    ts::verify_variants_equivalent<ce_cmp::OriginalArtOrgan,      ce_03a::OriginalArtSearchAlgo>(200u, 255u);   // uint8
    ts::verify_variants_equivalent<ce_cmp::OriginalHotOrgan,      ce_03a::OriginalHotSearchAlgo>(200u, 255u);   // uint8
    ts::verify_variants_equivalent<ce_cmp::OriginalWormholeOrgan, ce_03a::OriginalWormholeSearchAlgo>(200u, 255u); // uint8
    ts::verify_variants_equivalent<ce_cmp::OriginalSurfOrgan,     ce_03a::OriginalSurfSearchAlgo>(200u, 255u);  // uint8
    SUCCEED();  // ART/HOT/Wormhole/SuRF (uint8-IST-Bodies) exakt aus ihrer Organ-Komposition wiederherstellbar
}

// START ist das EINZIGE uint16-OriginalXxx-Tier → key_mod=1000 (sonst Key-Cast-Kollision organ=uint64 vs tier=uint16).
TEST(Axis03aTierOrgan, Uint16OriginalStartReconstructibleFromOrgan) {
    ts::verify_variants_equivalent<ce_cmp::OriginalStartOrgan, ce_03a::OriginalStartSearchAlgo>(1000u, 1000u);
    SUCCEED();  // START (uint16-IST-Body) exakt aus SortedBinaryOrgan wiederherstellbar
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

// --- ART-Trie echte Anatomie (#43 s4 Inc2-5) -------------------------------------------------------------
// Der OriginalArtOrgan ist jetzt das ECHTE ART-Organ (ComposedArtTrieSearch: adaptive Node4/16/48/256 +
// ByteWise-Path-Compression + Byte-Descent). Der uint8-Beleg (ART ≡ OriginalArtSearchAlgo) laeuft bereits in
// Uint8OriginalTiersReconstructibleFromOrgan oben. Hier zusaetzlich MULTI-BYTE-uint64-Belege, die Descent +
// Prefix-Split + Knoten-Growth (N4->N256) erst richtig ausreizen (uint8-Keys belegen nur den Wurzel-Knoten).
namespace {
template <class Organ>
void art_cross_check(Organ const& organ, std::map<std::uint64_t, std::uint64_t> const& ref, std::uint64_t query_max) {
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (std::uint64_t q = 0; q <= query_max; ++q) {
        auto const o  = organ.lookup(q);
        auto const it = ref.find(q);
        if (it == ref.end()) { ASSERT_FALSE(o.has_value()) << "phantom key " << q; }
        else { ASSERT_TRUE(o.has_value()) << "lost key " << q; ASSERT_EQ(*o, it->second) << "value mismatch key " << q; }
    }
}
}  // namespace

// Vertikal: ART-Organ ≡ std::map ueber einen MEHR-BYTE-Schluesselraum (Descent + Prefix + Growth via Harness-Stream).
TEST(Axis03aTierOrgan, Uint64ArtTrieMatchesStdMap) {
    ts::verify_matches_std_map<ce_cmp::ArtTrieOrgan>(60000u, 60000u);   // Keys spannen Byte 0+1 -> 2-Ebenen-Trie
    SUCCEED();
}

// Adversarial: erzwingt mehrstufigen Trie + N4->N256-Growth an Verzweigungen + Prefix-Split + Erase.
TEST(Axis03aTierOrgan, ArtTrieMultiLevelStressMatchesStdMap) {
    ce_cmp::ArtTrieOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;

    // (1) 0..1199: byte0 deckt 0..255 (Wurzel waechst auf N256), je byte0 bis 5 byte1-Werte (innere N4/N16)
    //     -> 2-Ebenen-Trie + Leaf-Split + Prefix-Split.
    for (std::uint64_t k = 0; k < 1200; ++k) { organ.insert(k, k * 3 + 1); ref[k] = k * 3 + 1; }
    art_cross_check(organ, ref, 1300);

    // (2) Hohe Bytes: Schluessel mit gesetztem Byte 2/3 -> tiefere Prefix-Ketten.
    for (std::uint64_t hi = 1; hi <= 8; ++hi) {
        std::uint64_t const k = (hi << 16) | 0x0102u;   // byte0=0x02, byte1=0x01, byte2=hi
        organ.insert(k, k); ref[k] = k;
    }
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()); ASSERT_EQ(*o, kv.second); }

    // (3) Update bestehender Keys (kein Count-Wachstum).
    for (std::uint64_t k = 0; k < 1200; k += 3) { organ.insert(k, k * 7); ref[k] = k * 7; }
    art_cross_check(organ, ref, 1300);

    // (4) Erase jeden 2. (remove_child ueber N256/N4 + Pfad-Cleanup), dann Kreuzpruefung.
    for (std::uint64_t k = 0; k < 1200; k += 2) { organ.erase(k); ref.erase(k); }
    art_cross_check(organ, ref, 1300);

    // (5) Rest leeren (absteigend) -> leerer Trie, dann Re-Insert (Free-List-Recycling).
    std::vector<std::uint64_t> rest;
    for (auto const& kv : ref) rest.push_back(kv.first);
    for (auto it = rest.rbegin(); it != rest.rend(); ++it) { organ.erase(*it); ref.erase(*it); }
    ASSERT_EQ(organ.occupied_count(), 0u);
    for (std::uint64_t k = 100; k < 400; ++k) { organ.insert(k, k + 9); ref[k] = k + 9; }
    art_cross_check(organ, ref, 500);
}
