#pragma once
// #188-4b/#188-4a -- Mapping organ-backed SearchAlgo -> sein NATIVES Composed*Search-Organ.
//
// @topic traversal @achse 03a @schicht composable
//
// **Zweck (Sibling zu `traversal_for_search_algo<S>`):** Fuer einen nicht-FLAT-store-traversierbaren,
// aber organ-backed Such-Algo liefert dieses Mapping das NATIVE `Composed*Search<...>`-Organ, mit dem
// `abi_adapter.hpp` das `container_t` parametrisiert: `ObservableComposedContainer<organ_for_search_algo_t<...>>`
// statt flachem SortedBinary-Spiegel ueber `LayoutAwareChunkedStore`. Gruppe (2) besteht seit #188-4a aus den
// 9 Pool-Familien plus der Eytzinger-Layout-Familie: EytzingerLayoutStore haelt sortierten Primaerzustand und
// abgeleiteten BFS-Puffer in EINEM Store (Option b, lazy rebuild).
//
// **TREUE (yield = ROHES Organ):** wie `traversal_for_search_algo` das rohe Traversal liefert, liefert dieses
// Mapping das rohe `Composed*Search`-Organ; der Adapter wrappt es mit `ObservableComposedContainer` (= die
// bestehenden `Observable*Organ`-Aliase in `tier_to_organ_mapping.hpp`, ObservableAxis + SearchAlgoStatistics +
// CoW `restore_statistics`). Alles AUSSER den 10 organ-backed Familien -> primaeres Template `void`. Das bleibt
// disjunkt zum Store-Traversal-Trait:
// (1) store-traversierbare Flach-Wrapper/Such-METHODEN (Array256/Array65535/VectorU8U8/VectorU16U16 seit #188-4c-ii,
// LinearScan/Interpolation/k-ary/per-K) mit faithful-Traversal via `traversal_for_search_algo`;
// (2) 9 Pool-Familien + Eytzinger-Layout-Familie mit nativem Organ HIER;
// (3) deferred/unregistrierte Typen = primaer `void` in diesem Trait, ggf. auch im Traversal-Trait. Belastbar
// (self-proving, s. unten): die zwei Sibling-Traits sind DISJUNKT.
//
// **Additiv:** formalisiert die im Aequivalenz-Test (`TierOrganPair`, `tier_to_organ_mapping.hpp`) belegte
// Tier->Organ-Zuordnung als compile-time-Trait, damit `abi_adapter.hpp` die container_t-Naht je organ-backed
// Familie umstellen kann. Wrapper-/Organ-Namen 1:1 aus Registry + Mapping verifiziert (kein Raten).
#include "tier_to_organ_mapping.hpp"     // Organ-Aliase: ArtTrieOrgan/HotPatriciaOrgan/StartTrieOrgan/WormholeOrgan/
                                         // SurfMapOrgan/SkipListOrgan/HashSearchOrgan/BstTreeOrgan/BTreeSearchOrgan
#include "traversal_for_search_algo.hpp" // traversal_for_search_algo_t + LinearScanSearchAlgo-Fwd (Partitions-Beleg)

#include <type_traits> // std::is_same_v (self-proving static_asserts — kein Raten)

namespace comdare::cache_engine::lookup {

// Vorwärts-Deklaration der 9 registrierten Weg-B-Pool-Familien-Wrapper (vermeidet Voll-Include der Wrapper-Header
// -> keine Zirkularität; identisch zum Muster in `traversal_for_search_algo.hpp`). Namen 1:1 aus AllStrategies:
// 5 OriginalXxx (Trie/Hybrid, S04-S08) + 4 CE-native (S13/S14/S16/S17). Masstree = DEFERRED (kein Wrapper in
// AllStrategies -> keine Spezialisierung; Organ-Alias existiert, wird bei Registrierung ergänzt).
class OriginalArtSearchAlgo;      // S04 ART (Leis ICDE 2013)
class OriginalHotSearchAlgo;      // S05 HOT (Binna PVLDB 2018)
class OriginalStartSearchAlgo;    // S06 START (Fent/Jungmair/Kipf/Neumann ICDEW 2020)
class OriginalWormholeSearchAlgo; // S07 Wormhole (Wu/Ni/Jiang ATC 2019)
class OriginalSurfSearchAlgo;     // S08 SuRF (Zhang/Lim/Andersen SIGMOD 2018) — Map-Schale
class SkipListSearchAlgo;         // S13 Skip-Liste (Pugh CACM 1990)
class HashSearchAlgo;             // S14 open-addressing Hash (Knuth TAOCP 3)
class BinarySearchTreeSearchAlgo; // S16 unbalancierter BST (Hibbard/Knuth)
class BTreeSearchAlgo;            // S17 B-Baum (Bayer/McCreight 1972, t=4)
class EytzingerSearchAlgo;        // S12 Eytzinger (Khuong/Morin JEA 2017)
// Repräsentant der store-traversierbaren Flach-Wrapper seit #188-4c-ii (organ_for bleibt void, traversal_for non-void).
class Array256SearchAlgo; // S01 direkt-adressiert std::array<optional,256> (ART Node256-Baseline, Leis ICDE 2013)
// Review-N7 (#188-4c-ii): die drei uebrigen Flach-Wrapper als explizite Regressions-Guards (asserts unten).
class Array65535SearchAlgo;
class VectorU8U8SearchAlgo;
class VectorU16U16SearchAlgo;

namespace composable {

// Primaer: kein natives Organ -> void. Betrifft: (a) store-traversierbare Flach-Wrapper/Such-METHODEN
// (faithful-Traversal via `traversal_for_search_algo`, aber KEIN Pool-Organ HIER); (b) deferred/unregistriertes
// Masstree und sonstige nicht gemappte Typen.
template <class S>
struct organ_for_search_algo {
    using type = void;
};

// --- Die 9 registrierten Weg-B-Pool-Familien -> ihr treues Composed*Search<Traversal,*NodePoolStore>-Organ.
// (yield = ROHES Organ; 4b-b wrappt mit ObservableComposedContainer, s. tier_to_organ_mapping.hpp:86-97.)
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::OriginalArtSearchAlgo> {
    using type = ArtTrieOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::OriginalHotSearchAlgo> {
    using type = HotPatriciaOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::OriginalStartSearchAlgo> {
    using type = StartTrieOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::OriginalWormholeSearchAlgo> {
    using type = WormholeOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::OriginalSurfSearchAlgo> {
    using type = SurfMapOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::SkipListSearchAlgo> {
    using type = SkipListOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::HashSearchAlgo> {
    using type = HashSearchOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo> {
    using type = BstTreeOrgan;
};
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::BTreeSearchAlgo> {
    using type = BTreeSearchOrgan;
};
// #188-4a: Eytzinger ist die 10. organ_for-Familie. Kein Pool: EytzingerLayoutStore = sortierte Basis +
// abgeleiteter BFS-Puffer in EINEM Store (Option b, lazy rebuild). traversal_for bleibt bewusst void.
template <>
struct organ_for_search_algo<::comdare::cache_engine::lookup::EytzingerSearchAlgo> {
    using type = EytzingerOrgan;
};

template <class S>
using organ_for_search_algo_t = typename organ_for_search_algo<S>::type;

// --- Verifikation (kein Raten): die 9 Pool-Familien mappen je auf ihr treues natives Organ.
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalArtSearchAlgo>, ArtTrieOrgan>,
    "#188-4b-a: ART -> ArtTrieOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalHotSearchAlgo>, HotPatriciaOrgan>,
    "#188-4b-a: HOT -> HotPatriciaOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalStartSearchAlgo>, StartTrieOrgan>,
    "#188-4b-a: START -> StartTrieOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalWormholeSearchAlgo>, WormholeOrgan>,
    "#188-4b-a: Wormhole -> WormholeOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalSurfSearchAlgo>, SurfMapOrgan>,
    "#188-4b-a: SuRF -> SurfMapOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::SkipListSearchAlgo>, SkipListOrgan>,
    "#188-4b-a: SkipList -> SkipListOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::HashSearchAlgo>, HashSearchOrgan>,
              "#188-4b-a: Hash -> HashSearchOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>, BstTreeOrgan>,
    "#188-4b-a: BST -> BstTreeOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::BTreeSearchAlgo>, BTreeSearchOrgan>,
    "#188-4b-a: BTree -> BTreeSearchOrgan");
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::EytzingerSearchAlgo>, EytzingerOrgan>,
    "#188-4a: Eytzinger -> EytzingerOrgan");

// --- Disjunktheit (self-proving): kein Wrapper ist in BEIDEN Sibling-Traits non-void. Belegt an Repraesentanten:
// (1) Pool-Familie BST = Organ non-void, faithful-Traversal void; (2) Such-METHODE linear_scan = Traversal non-void,
// Organ void; (3) Flach-Wrapper Array256 seit #188-4c-ii = faithful DirectAddressTraversal non-void, Organ void.
static_assert(
    std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>, void>,
    "#188-4b-a Disjunktheit: BST = Pool-Familie -> KEIN faithful-Traversal (void)");
static_assert(
    !std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>, void>,
    "#188-4b-a Disjunktheit: BST = Pool-Familie -> HAT natives Organ (non-void)");
static_assert(!std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::LinearScanSearchAlgo>, void>,
              "#188-4b-a Disjunktheit: linear_scan = Such-METHODE -> HAT faithful-Traversal (non-void)");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::LinearScanSearchAlgo>, void>,
              "#188-4b-a Disjunktheit: linear_scan = Such-METHODE -> KEIN Pool-Organ (void)");
// #188-4c-ii: direkt-adressierter Flach-Array-Wrapper ist store-traversierbar (faithful Traversal non-void), aber
// weiter KEIN Pool-/organ_for-Familienmitglied. Das beweist die Disjunktheit der Sibling-Traits fuer die Flachgruppe.
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>, void>,
              "#188-4c-ii: Array256 = Flach-Array -> KEIN Pool-Organ (organ_for void)");
static_assert(!std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>, void>,
              "#188-4c-ii: Array256 = Flach-Array -> faithful DirectAddressTraversal (traversal_for non-void)");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>,
                             DirectAddressTraversal>,
              "#188-4c-ii: Array256 -> DirectAddressTraversal");
// Review-N7 (#188-4c-ii): Regressions-Guards fuer die drei uebrigen Flach-Wrapper — organ_for bleibt void
// (store-traversierbar via traversal_for, KEIN Pool-Organ); explizit belegt statt implizit am Primary Template.
static_assert(
    std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::Array65535SearchAlgo>, void> &&
        std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::VectorU8U8SearchAlgo>, void> &&
        std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::VectorU16U16SearchAlgo>, void>,
    "#188-4c-ii: alle 4 Flach-Wrapper bleiben organ_for==void (store-traversierbar, kein Pool-Organ)");
static_assert(!std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::EytzingerSearchAlgo>, void>,
              "#188-4a 3-Wege-Split: Eytzinger = organ-backed Layout-Familie -> HAT natives Organ");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::EytzingerSearchAlgo>, void>,
              "#188-4a 3-Wege-Split: Eytzinger -> KEIN faithful FLAT-Store-Traversal");

} // namespace composable
} // namespace comdare::cache_engine::lookup
