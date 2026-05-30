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
#include <topics/traversal/axis_03a_search_algo/composable/masstree_layer_pool_store.hpp>     // Masstree-Kperm15 (S9-Fundament)
#include <topics/traversal/axis_03a_search_algo/composable/composed_masstree_search.hpp>      // Masstree-Organ (S3/S4)
#include "support/std_map_equivalence_harness.hpp"

#include <map>
#include <vector>

namespace ce_03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ce_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
namespace ts     = ::comdare::cache_engine::test_support;

// --- Masstree-Fundament (S9): kpermuter-Bit-Arithmetik (fragilste Stelle) VOR dem Organ isoliert verifiziert.
// Belegt Kperm15 gegen die dokumentierten kpermuter.hh-Invarianten (make_sorted/insert_from_back/remove).
TEST(MasstreeKpermuter, Kperm15Invariants) {
    using KP = ce_cmp::MasstreeLayerNodePoolStore::Kperm15;
    // make_sorted(n): size()==n und [i]==i fuer 0<=i<n.
    for (int n = 0; n <= 15; ++n) {
        KP p{KP::make_sorted(n)};
        ASSERT_EQ(p.size(), n) << "make_sorted size n=" << n;
        for (int i = 0; i < n; ++i) ASSERT_EQ(p[i], i) << "make_sorted[" << i << "] n=" << n;
    }
    // make_empty: size 0; Allokationsreihenfolge 0,1,2,... via insert_from_back(0) (Prepend -> [k-1..0]).
    {
        KP p{KP::make_empty()};
        ASSERT_EQ(p.size(), 0);
        for (int k = 0; k < 5; ++k) { int const phys = p.insert_from_back(0); ASSERT_EQ(phys, k) << "alloc order k=" << k; }
        ASSERT_EQ(p.size(), 5);
        for (int i = 0; i < 5; ++i) ASSERT_EQ(p[i], 4 - i) << "prepend[" << i << "]";
    }
    // insert_from_back(i) Invariante: q[j]==p[j] (j<i); q[i]==alloc; q[j]==p[j-1] (i<j<size).
    {
        KP p{KP::make_sorted(4)};                 // [0,1,2,3], back()==4
        int const x = p.insert_from_back(2);
        ASSERT_EQ(x, 4);
        ASSERT_EQ(p.size(), 5);
        ASSERT_EQ(p[0], 0); ASSERT_EQ(p[1], 1); ASSERT_EQ(p[2], 4); ASSERT_EQ(p[3], 2); ASSERT_EQ(p[4], 3);
    }
    // remove(i) Invariante: size-1; [j]==alt[j] (j<i); [j]==alt[j+1] (i<=j<size); entfernter Slot -> back.
    {
        KP p{KP::make_sorted(5)};                 // [0,1,2,3,4]
        p.remove(2);
        ASSERT_EQ(p.size(), 4);
        ASSERT_EQ(p[0], 0); ASSERT_EQ(p[1], 1); ASSERT_EQ(p[2], 3); ASSERT_EQ(p[3], 4);
        ASSERT_EQ(p[4], 2) << "entfernter Slot recycelbar am back";
    }
    {
        KP q{KP::make_sorted(3)};                 // [0,1,2]; remove letztes (Sonderfall x&15==i+1)
        q.remove(2);
        ASSERT_EQ(q.size(), 2);
        ASSERT_EQ(q[0], 0); ASSERT_EQ(q[1], 1);
    }
    SUCCEED();
}

// --- Masstree-Organ standalone: Voll-Dekompositions-B+Baum-of-Tries gegen std::map (S3/S4 Verifikation) ---
using MasstreeOrgan2 = ce_cmp::ComposedMasstreeSearch<ce_cmp::MasstreeLayerTraversalOrgan<2>, ce_cmp::MasstreeLayerNodePoolStore>;
using MasstreeOrgan8 = ce_cmp::ComposedMasstreeSearch<ce_cmp::MasstreeLayerTraversalOrgan<8>, ce_cmp::MasstreeLayerNodePoolStore>;
TEST(Axis03aTierOrgan, Uint64MasstreeMatchesStdMap) {
    ts::verify_matches_std_map<MasstreeOrgan2>(60000u, 60000u);   // Mehr-Layer (SliceBytes=2) + Leaf/Internode-Split + neue Wurzel
    ts::verify_matches_std_map<MasstreeOrgan8>(60000u, 60000u);   // Single-Layer-Degenerationsanker (SliceBytes=8 == reiner B+Baum)
    ts::verify_matches_std_map<MasstreeOrgan2>(200u, 255u);
    ts::verify_matches_std_map<MasstreeOrgan2>(1000u, 1000u);
}

// Namensanspruch-Beleg (Doku 14, [[algorithm-correctness-when-named]]): Keys mit IDENTISCHEM fuehrenden
// 2-Byte-Slice (slice[0] fix) + divergentem Rest erzwingen echte Sub-Layer (B+Baum-of-Tries, nicht flacher
// B+Baum). Ohne korrekte Layer-Maschinerie ginge bei slice[0]-Kollision der Lookup verloren.
TEST(Axis03aTierOrgan, MasstreeLayerBoundaryMatchesStdMap) {
    MasstreeOrgan2 organ;
    std::map<std::uint64_t, std::uint64_t> ref;
    auto mk = [](std::uint64_t i) { return (std::uint64_t{0x0042} << 48) | ((i * 2654435761u) % 0x0000FFFFFFFFFFFFull); };
    for (std::uint64_t i = 0; i < 1500; ++i) { std::uint64_t const k = mk(i); organ.insert(k, k * 11 + 1); ref[k] = k * 11 + 1; }
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()) << kv.first; ASSERT_EQ(*o, kv.second); }
    EXPECT_FALSE(organ.lookup((std::uint64_t{0x0043} << 48)).has_value());   // anderer slice[0] -> nicht da
    for (std::uint64_t i = 0; i < 1500; i += 2) { std::uint64_t const k = mk(i); organ.erase(k); ref.erase(k); }
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()); ASSERT_EQ(*o, kv.second); }
    SUCCEED();
}

// Horizontal: Masstree-Organ ≡ ArtTrieOrgan (identischer Op-Stream, identische Resultate) -> transitiv ≡ std::map.
TEST(Axis03aTierOrgan, MasstreeVariantsEquivalentToArt) {
    ts::verify_variants_equivalent<MasstreeOrgan2, ce_cmp::ArtTrieOrgan>(60000u, 60000u);
    SUCCEED();
}

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
    // #42 Phase 2: die OriginalXxx-Tier-Wrapper sind nach Deregistrierung No-op-Stubs (enabled=false,
    // Compile-Time-Switch). Der Tier-vs-Organ-Rekonstruktionsbeweis ist nur sinnvoll, wenn das Tier AKTIV
    // ist (reversibel: -DCOMDARE_AXIS_03A_ENABLE_ORIGINAL_*=ON re-aktiviert ihn). Die Organ-vs-std::map-
    // Gleichheit (das bleibende Invariant) liegt separat in den verify_matches_std_map-Tests.
    if constexpr (ce_03a::OriginalArtSearchAlgo::enabled)
        ts::verify_variants_equivalent<ce_cmp::OriginalArtOrgan,      ce_03a::OriginalArtSearchAlgo>(200u, 255u);
    if constexpr (ce_03a::OriginalHotSearchAlgo::enabled)
        ts::verify_variants_equivalent<ce_cmp::OriginalHotOrgan,      ce_03a::OriginalHotSearchAlgo>(200u, 255u);
    if constexpr (ce_03a::OriginalWormholeSearchAlgo::enabled)
        ts::verify_variants_equivalent<ce_cmp::OriginalWormholeOrgan, ce_03a::OriginalWormholeSearchAlgo>(200u, 255u);
    if constexpr (ce_03a::OriginalSurfSearchAlgo::enabled)
        ts::verify_variants_equivalent<ce_cmp::OriginalSurfOrgan,     ce_03a::OriginalSurfSearchAlgo>(200u, 255u);
    SUCCEED();  // ART/HOT/Wormhole/SuRF aus Organ-Komposition wiederherstellbar (sofern Tier aktiv)
}

// START ist das EINZIGE uint16-OriginalXxx-Tier → key_mod=1000 (sonst Key-Cast-Kollision organ=uint64 vs tier=uint16).
TEST(Axis03aTierOrgan, Uint16OriginalStartReconstructibleFromOrgan) {
    if constexpr (ce_03a::OriginalStartSearchAlgo::enabled)  // #42 Phase 2: deregistriert -> Stub, skip
        ts::verify_variants_equivalent<ce_cmp::OriginalStartOrgan, ce_03a::OriginalStartSearchAlgo>(1000u, 1000u);
    SUCCEED();  // START aus Multibyte-Span-Organ wiederherstellbar (sofern Tier aktiv)
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

// --- HOT echte bit-Patricia-Anatomie (#43 s4) ----------------------------------------------------------
// OriginalHotOrgan ist jetzt das ECHTE HOT-Organ (ComposedHotPatriciaSearch: binary crit-bit, MSB-first,
// Single-Bit-Split + Collapse-Erase). Der uint8-Beleg (HOT ≡ OriginalHotSearchAlgo) laeuft in
// Uint8OriginalTiersReconstructibleFromOrgan oben; uint8-Keys belegen MSB-first nur eine flache Patricia,
// daher hier zusaetzlich MULTI-BIT-uint64-Belege (mehrstufige Patricia + Single-Bit-Split + Collapse).
TEST(Axis03aTierOrgan, Uint64HotPatriciaMatchesStdMap) {
    ts::verify_matches_std_map<ce_cmp::HotPatriciaOrgan>(60000u, 60000u);
    SUCCEED();
}

TEST(Axis03aTierOrgan, HotPatriciaMultiLevelStressMatchesStdMap) {
    ce_cmp::HotPatriciaOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;

    // (1) tiefe Patricia + viele Single-Bit-Splits.
    for (std::uint64_t k = 0; k < 1200; ++k) { organ.insert(k, k * 3 + 1); ref[k] = k * 3 + 1; }
    art_cross_check(organ, ref, 1300);
    // (2) hohe Bytes -> lange gemeinsame MSB-Prefixe, tiefe Descent-Ketten.
    for (std::uint64_t hi = 1; hi <= 8; ++hi) { std::uint64_t const k = (hi << 16) | 0x0102u; organ.insert(k, k); ref[k] = k; }
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()); ASSERT_EQ(*o, kv.second); }
    // (3) Update-Pfad (a==b) — KEIN Count-Wachstum.
    for (std::uint64_t k = 0; k < 1200; k += 3) { organ.insert(k, k * 7); ref[k] = k * 7; }
    art_cross_check(organ, ref, 1300);
    // (4) Erase jeden 2. -> Collapse + Doppel-free + Free-List-Recycling.
    for (std::uint64_t k = 0; k < 1200; k += 2) { organ.erase(k); ref.erase(k); }
    art_cross_check(organ, ref, 1300);
    // (5) Rest absteigend leeren -> root==NIL, dann Re-Insert (Recycling beider Kinds).
    std::vector<std::uint64_t> rest;
    for (auto const& kv : ref) rest.push_back(kv.first);
    for (auto it = rest.rbegin(); it != rest.rend(); ++it) { organ.erase(*it); ref.erase(*it); }
    ASSERT_EQ(organ.occupied_count(), 0u);
    for (std::uint64_t k = 100; k < 400; ++k) { organ.insert(k, k + 9); ref[k] = k + 9; }
    art_cross_check(organ, ref, 500);

    // ZUSATZ — Patricia-Extreme (Bit-Grenzfaelle ueber den ART-Klon hinaus).
    organ.clear(); ref.clear();
    organ.insert(0, 1); ref[0] = 1; organ.insert(1, 2); ref[1] = 2;                          // crit_bit=63 (tiefste Stelle)
    organ.insert(0x8000000000000000ULL, 3); ref[0x8000000000000000ULL] = 3;                  // crit_bit=0 (Wurzel-Split)
    for (int s = 0; s < 16; ++s) { std::uint64_t const k = (1ULL << s) - 1; organ.insert(k, k + 1); ref[k] = k + 1; }  // 0,0x1,0x3,0x7,... Praefix-Kette
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()) << kv.first; ASSERT_EQ(*o, kv.second); }
    ASSERT_FALSE(organ.lookup(0x4000000000000000ULL).has_value());                           // Phantom-Negativprobe
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

// Regression (adversariale Verifikation w21detpyz): ART-N48-Slot-Reuse nach Erase. Haelt die Wurzel in der
// N48-Groesse (17..48 Kinder) waehrend Erase+Reinsert-Zyklen — der ArtTrieMultiLevelStress waechst vorher auf
// N256 und verfehlte daher das N48-Slot-Aliasing-Fenster. Keys mit distinktem Byte 0 (hoehere Bytes 0) -> EIN N48.
TEST(Axis03aTierOrgan, ArtTrieN48EraseReinsertMatchesStdMap) {
    ce_cmp::ArtTrieOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;
    // Wurzel auf N48 fuellen (40 Kinder: N4->N16->N48).
    for (std::uint64_t k = 1; k <= 40; ++k) { organ.insert(k, k + 1000); ref[k] = k + 1000; }
    art_cross_check(organ, ref, 300);
    // 30 Erase+Reinsert-Zyklen — Knoten bleibt N48 (immer ~40 Kinder) -> exerziert den Slot-Recycling-Pfad.
    for (std::uint64_t c = 0; c < 30; ++c) {
        std::uint64_t const victim = 1 + (c % 40);
        if (ref.count(victim)) { organ.erase(victim); ref.erase(victim); }
        std::uint64_t const fresh = 100 + c;                 // distinkte Byte-0-Werte 100..129
        organ.insert(fresh, fresh + 1); ref[fresh] = fresh + 1;
        art_cross_check(organ, ref, 300);                     // JEDER Zyklus gegen std::map
    }
    // Danach auf N256 wachsen (distinkte Byte-0-Werte 130..255) + dort weiter erase.
    for (std::uint64_t k = 130; k <= 255; ++k) { organ.insert(k, k); ref[k] = k; }
    art_cross_check(organ, ref, 300);
    for (std::uint64_t k = 130; k <= 255; k += 2) { organ.erase(k); ref.erase(k); }
    art_cross_check(organ, ref, 300);
}

// Regression (adversariale Verifikation): kleiner Keyraum + viele Ops -> starkes Churn (Update + Erase-dann-
// Reinsert derselben Keys) -> ein MERGE kann ein Leaf auf kWhKpn fuellen, ein Folge-Insert haette ohne den
// Array-Kapazitaet-kWhKpn+1-Fix das Leaf ueberlaufen lassen (op 307 im Harness-Stream). key_mod=200/250/300.
TEST(Axis03aTierOrgan, WormholeSmallKeyChurnMatchesStdMap) {
    ts::verify_matches_std_map<ce_cmp::WormholeOrgan>(200u, 255u);
    ts::verify_matches_std_map<ce_cmp::WormholeOrgan>(50u, 60u);     // noch dichteres Churn (jeder Key ~12x)
    ts::verify_matches_std_map<ce_cmp::WormholeOrgan>(300u, 320u);
    SUCCEED();
}

// --- Wormhole echte Hybrid-Anatomie (#43 s4) -----------------------------------------------------------
// OriginalWormholeOrgan ist jetzt das ECHTE Wormhole-Organ (ComposedWormholeSearch: sortierte doppelt-
// verkettete Leaf-Liste + Hash-Anchor-Jump + Leaf-Split/Merge). Der uint8-Beleg (Wormhole ≡
// OriginalWormholeSearchAlgo) laeuft in Uint8OriginalTiersReconstructibleFromOrgan oben; hier zusaetzlich
// uint64-Multi-Leaf-Belege (kWhKpn=8 -> viele Splits/Merges + Anchor-Pflege via Jump-Index).
TEST(Axis03aTierOrgan, Uint64WormholeMatchesStdMap) {
    ts::verify_matches_std_map<ce_cmp::WormholeOrgan>(60000u, 60000u);
    SUCCEED();
}

// SuRF-Map-Schale (#43 s4): autoritatives exaktes K->V == std::map (das LOUDS-Filter-Organ in axis_filter
// liefert nur approximatives may-contain; die Map-Schale traegt die exakte Vergleichbarkeit). uint8-Beleg
// (SurfMapOrgan == OriginalSurfSearchAlgo) laeuft in Uint8OriginalTiersReconstructibleFromOrgan oben.
TEST(Axis03aTierOrgan, Uint64SurfMapMatchesStdMap) {
    ts::verify_matches_std_map<ce_cmp::SurfMapOrgan>(60000u, 60000u);
    ts::verify_matches_std_map<ce_cmp::SurfMapOrgan>(200u, 255u);    // dichtes Churn (Update + Erase-Reinsert)
    SUCCEED();
}

TEST(Axis03aTierOrgan, WormholeMultiLevelStressMatchesStdMap) {
    ce_cmp::WormholeOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;

    // (1) 0..1199 -> viele Leaf-Splits (kWhKpn=8), lange Liste.
    for (std::uint64_t k = 0; k < 1200; ++k) { organ.insert(k, k * 3 + 1); ref[k] = k * 3 + 1; }
    art_cross_check(organ, ref, 1300);
    // (2) hohe Bytes weit auseinander -> Hash-Jump ueber grosse Anchor-Luecken.
    for (std::uint64_t hi = 1; hi <= 16; ++hi) { std::uint64_t const k = (hi << 48) | 0x0102u; organ.insert(k, k); ref[k] = k; }
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()); ASSERT_EQ(*o, kv.second); }
    // (3) Update jeden 3. (a==a) — KEIN Count-Wachstum.
    for (std::uint64_t k = 0; k < 1200; k += 3) { organ.insert(k, k * 7); ref[k] = k * 7; }
    art_cross_check(organ, ref, 1300);
    // (4) Erase jeden 2. -> verstreuter Borrow/Merge + Anchor-Veraltung beim Minimum-Erase.
    for (std::uint64_t k = 0; k < 1200; k += 2) { organ.erase(k); ref.erase(k); }
    art_cross_check(organ, ref, 1300);
    // (5) Rest absteigend leeren -> root==kNil, dann Re-Insert (Free-List-Recycling + Listen-Neuaufbau).
    std::vector<std::uint64_t> rest;
    for (auto const& kv : ref) rest.push_back(kv.first);
    for (auto it = rest.rbegin(); it != rest.rend(); ++it) { organ.erase(*it); ref.erase(*it); }
    ASSERT_EQ(organ.occupied_count(), 0u);
    for (std::uint64_t k = 100; k < 400; ++k) { organ.insert(k, k + 9); ref[k] = k + 9; }
    art_cross_check(organ, ref, 500);

    // ZUSATZ R1 — Anchor-Veraltung: wiederholt Minimum loeschen + neues kleineres globales Minimum einfuegen
    // (erzwingt reanchor(head) + Index-Pflege + Listen-Kopf-Verschiebung).
    organ.clear(); ref.clear();
    for (std::uint64_t k = 1000; k < 1100; ++k) { organ.insert(k, k); ref[k] = k; }
    art_cross_check(organ, ref, 1200);
    for (std::uint64_t c = 0; c < 50; ++c) {
        std::uint64_t const cur_min = ref.begin()->first;
        organ.erase(cur_min); ref.erase(cur_min);
        std::uint64_t const newk = 900 - c;            // streng fallend, unter allen vorhandenen
        organ.insert(newk, newk + 1); ref[newk] = newk + 1;
        art_cross_check(organ, ref, 1200);
    }
}

// --- START echte Multibyte-Span-Anatomie (#43 s4, letztes Tier) ----------------------------------------
// OriginalStartOrgan ist jetzt das ECHTE START-Organ (ComposedStartTrieSearch span-2: Multibyte-Span-Radix
// mit per-Node-Span + ByteWise-Path-Compression). uint16-Beleg (START == OriginalStartSearchAlgo) laeuft in
// Uint16OriginalStartReconstructibleFromOrgan oben; hier MULTI-BYTE-uint64-Belege (span-2-Diskriminatoren).
using StartSpan1Organ = ce_cmp::ComposedStartTrieSearch<ce_cmp::StartTrieTraversalOrgan<1>, ce_cmp::StartTrieNodePoolStore>;

TEST(Axis03aTierOrgan, Uint64StartTrieMatchesStdMap) {
    ts::verify_matches_std_map<ce_cmp::StartTrieOrgan>(60000u, 60000u);   // span-2 Multibyte-Diskriminatoren
    ts::verify_matches_std_map<ce_cmp::StartTrieOrgan>(200u, 255u);       // dichtes uint8-Churn
    SUCCEED();
}

// I1-Anker: span-1-START ist verhaltensaequivalent zu ART (degeneriert zum Byte-Radix) — beide == std::map.
TEST(Axis03aTierOrgan, StartSpan1EquivalentToArt) {
    ts::verify_variants_equivalent<StartSpan1Organ, ce_cmp::ArtTrieOrgan>(60000u, 60000u);
    ts::verify_variants_equivalent<StartSpan1Organ, ce_cmp::ArtTrieOrgan>(200u, 255u);
    SUCCEED();
}

// Adversarial: span-2-Diskriminatoren (Keys divergieren im 2./3. Byte) + Multi-Level + Erase-Reinsert.
TEST(Axis03aTierOrgan, StartTrieMultiByteStressMatchesStdMap) {
    ce_cmp::StartTrieOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;
    // (1) 0..1199 -> span-2-Knoten branchen ueber 2 Bytes (byte0+byte1).
    for (std::uint64_t k = 0; k < 1200; ++k) { organ.insert(k, k * 3 + 1); ref[k] = k * 3 + 1; }
    art_cross_check(organ, ref, 1300);
    // (2) Keys, die NUR im 2./3. Byte divergieren (gemeinsamer Byte-0-Prefix -> Prefix-Compression + span-Disk).
    for (std::uint64_t hi = 0; hi < 64; ++hi) { std::uint64_t const k = (hi << 8) | 0x07u; organ.insert(k, k + 5); ref[k] = k + 5; }
    for (std::uint64_t hi = 1; hi <= 12; ++hi) { std::uint64_t const k = (hi << 16) | 0x0203u; organ.insert(k, k); ref[k] = k; }
    art_cross_check(organ, ref, 1300);
    // (3) Update (a==a) — kein Count-Wachstum.
    for (std::uint64_t k = 0; k < 1200; k += 3) { organ.insert(k, k * 9); ref[k] = k * 9; }
    art_cross_check(organ, ref, 1300);
    // (4) Erase jeden 2. -> remove_child ueber span-Disk + Free-List-Recycling.
    for (std::uint64_t k = 0; k < 1200; k += 2) { organ.erase(k); ref.erase(k); }
    art_cross_check(organ, ref, 1300);
    // (5) Rest absteigend leeren -> root==kNil, Re-Insert.
    std::vector<std::uint64_t> rest;
    for (auto const& kv : ref) rest.push_back(kv.first);
    for (auto it = rest.rbegin(); it != rest.rend(); ++it) { organ.erase(*it); ref.erase(*it); }
    ASSERT_EQ(organ.occupied_count(), 0u);
    for (std::uint64_t k = 100; k < 500; ++k) { organ.insert(k, k + 9); ref[k] = k + 9; }
    art_cross_check(organ, ref, 600);
}

// Coverage-Haertung (adversariale Verifikation w3346v581): span-3 (Rewired16M, 24-Bit-Diskriminator) +
// hohe Byte-Positionen (5/6/7) + Phantom-Byte-7-Grenze — vom Stress oben nicht abgedeckte Korrektheitsflaeche.
using StartSpan3Organ = ce_cmp::ComposedStartTrieSearch<ce_cmp::StartTrieTraversalOrgan<3>, ce_cmp::StartTrieNodePoolStore>;
TEST(Axis03aTierOrgan, StartSpan3AndHighByteMatchesStdMap) {
    ts::verify_matches_std_map<StartSpan3Organ>(60000u, 60000u);   // span-3 gegen std::map
    ts::verify_matches_std_map<StartSpan3Organ>(200u, 255u);

    ce_cmp::StartTrieOrgan organ;
    std::map<std::uint64_t, std::uint64_t> ref;
    for (std::uint64_t v = 1; v <= 300; ++v) { std::uint64_t const k = (v << 48) | 0x0102u; organ.insert(k, k); ref[k] = k; }  // span-Kette ueber Byte 6/7
    organ.insert(0xAA00000000000001ull, 1); ref[0xAA00000000000001ull] = 1;   // Byte-7-Divergenz
    organ.insert(0xBB00000000000001ull, 2); ref[0xBB00000000000001ull] = 2;
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()) << kv.first; ASSERT_EQ(*o, kv.second); }
    for (std::uint64_t v = 1; v <= 300; v += 2) { std::uint64_t const k = (v << 48) | 0x0102u; organ.erase(k); ref.erase(k); }   // Erase ueber span-Disk
    ASSERT_EQ(organ.occupied_count(), ref.size());
    for (auto const& kv : ref) { auto o = organ.lookup(kv.first); ASSERT_TRUE(o.has_value()); ASSERT_EQ(*o, kv.second); }
    SUCCEED();
}
