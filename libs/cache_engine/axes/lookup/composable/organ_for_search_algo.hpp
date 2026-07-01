#pragma once
// #188-4b Increment 4b-a (2026-07-01) — Mapping Pool-Familien-Such-Algo -> sein NATIVES Composed*Search-Organ.
//
// @topic traversal @achse 03a @schicht composable
//
// **Zweck (Pool-Analogon zu `traversal_for_search_algo<S>`):** Für einen Weg-B-Pool-Such-Algo (Tree/Trie/Hash —
// NICHT store-traversierbar, `StoreTraversableSearchAlgo<S> == false`) liefert dieses Mapping das NATIVE
// `Composed*Search<Traversal,*NodePoolStore>`-Organ, mit dem #188-4b-b das `container_t` parametrisiert:
// `ObservableComposedContainer<organ_for_search_algo_t<Composition::search_algo>>` statt des flachen
// SortedBinary-Spiegels über `LayoutAwareChunkedStore`. Dadurch läuft die T0-Suche der Pool-Familie über ihren
// ECHTEN Knoten-Pool (kein Neben-Apparat) — die intra-Gattungs-Speicher-Organ-Zielformel (Thesis-fundiert,
// Codex-verifiziert 2026-07-01; `store_traversable_search_algo.hpp:16-22`: der search_organ_/container_-Split
// „ist ein BUG, KEIN gewolltes Design"). SearchAlgorithm bleibt eigene Gattung (kein Cross-Genus, keine
// „Container-Achse"): das Organ ist ein Speicher-Organ INNERHALB der Gattung.
//
// **TREUE (yield = ROHES Organ):** wie `traversal_for_search_algo` das rohe Traversal liefert, liefert dieses
// Mapping das rohe `Composed*Search`-Organ; 4b-b wrappt es mit `ObservableComposedContainer` (= die bestehenden
// `Observable*Organ`-Aliase in `tier_to_organ_mapping.hpp:86-97`, ObservableAxis + SearchAlgoStatistics + CoW
// `restore_statistics`). Alles AUSSER den 9 Pool-Familien → primäres Template `void`. Das ist ein 3-WEGE-Split
// (KEINE saubere 2-Wege-Partition — `StoreTraversableSearchAlgo` ist marker-basiert): (1) store-traversierbare
// Such-METHODEN (LinearScan/Interpolation/k-ary/per-K) mit faithful-Traversal via `traversal_for_search_algo`;
// (2) die 9 Weg-B-Pool-Familien mit nativem Organ HIER; (3) markerlose flache Array-/Vector-Wrapper (Array256/
// VectorU8U8/VectorU16U16/Array65535 — WEDER store-traversierbar-markiert NOCH pool-basiert → Default-Sorted-
// Binary-Pfad im container_t) + Eytzinger (#188-4a BFS-Layout) + deferred/unregistriertes Masstree = void in
// BEIDEN Traits. Belastbar (self-proving, s. unten): die zwei Sibling-Traits sind DISJUNKT (kein Wrapper ist in
// beiden non-void); die organ-gemappten 9 sind eine ECHTE Teilmenge der nicht-store-traversierbaren Menge.
//
// **Additiv (4b-a ändert NOCH KEIN Verhalten):** formalisiert nur die im Äquivalenz-Test (`TierOrganPair`,
// `tier_to_organ_mapping.hpp`) belegte Tier->Organ-Zuordnung als compile-time-Trait, damit 4b-b die container_t-
// Naht (`abi_adapter.hpp:1890-1897`) je Pool-Familie umstellen kann. Wrapper-/Organ-Namen 1:1 aus der Registry
// (`axis_03a_search_algo_registry.hpp` AllStrategies) + `tier_to_organ_mapping.hpp` verifiziert (kein Raten).

#include "tier_to_organ_mapping.hpp" // Organ-Aliase: ArtTrieOrgan/HotPatriciaOrgan/StartTrieOrgan/WormholeOrgan/
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
// Repräsentant der DRITTEN Gruppe (markerloser flacher Array-/Vector-Wrapper) — für den „void in BEIDEN Traits"-
// Beleg unten (weder store-traversierbar-markiert noch Pool-Familie; Default-SortedBinary-Pfad im container_t).
class Array256SearchAlgo; // S01 direkt-adressiert std::array<optional,256> (ART Node256-Baseline, Leis ICDE 2013)

namespace composable {

// Primär: kein natives Pool-Organ -> void. Betrifft: (a) Such-METHODEN (LinearScan/Interpolation/k-ary/per-K =
// store-traversierbar, faithful-Traversal via `traversal_for_search_algo`); (b) markerlose flache Array-/Vector-
// Wrapper (Array256/VectorU8U8/VectorU16U16/Array65535 = weder store-traversierbar-markiert noch Pool ->
// Default-SortedBinary); (c) Eytzinger (#188-4a BFS-Layout) + deferred/unregistriertes Masstree.
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

template <class S>
using organ_for_search_algo_t = typename organ_for_search_algo<S>::type;

// --- Verifikation (kein Raten): die 9 Pool-Familien mappen je auf ihr treues natives Organ.
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalArtSearchAlgo>,
                             ArtTrieOrgan>,
              "#188-4b-a: ART -> ArtTrieOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalHotSearchAlgo>,
                             HotPatriciaOrgan>,
              "#188-4b-a: HOT -> HotPatriciaOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalStartSearchAlgo>,
                             StartTrieOrgan>,
              "#188-4b-a: START -> StartTrieOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalWormholeSearchAlgo>,
                             WormholeOrgan>,
              "#188-4b-a: Wormhole -> WormholeOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::OriginalSurfSearchAlgo>,
                             SurfMapOrgan>,
              "#188-4b-a: SuRF -> SurfMapOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::SkipListSearchAlgo>,
                             SkipListOrgan>,
              "#188-4b-a: SkipList -> SkipListOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::HashSearchAlgo>,
                             HashSearchOrgan>,
              "#188-4b-a: Hash -> HashSearchOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>,
                             BstTreeOrgan>,
              "#188-4b-a: BST -> BstTreeOrgan");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::BTreeSearchAlgo>,
                             BTreeSearchOrgan>,
              "#188-4b-a: BTree -> BTreeSearchOrgan");

// --- Disjunktheit + 3-Wege-Split (self-proving; NICHT als 2-Wege-Partition überclaimen, s. Header): kein Wrapper
// ist in BEIDEN Sibling-Traits non-void. Belegt an je einem Repräsentanten jeder Gruppe: (1) Pool-Familie BST =
// Organ non-void, faithful-Traversal void; (2) Such-METHODE linear_scan = Traversal non-void, Organ void;
// (3) Flach-Array-Wrapper Array256 = in BEIDEN void (Default-SortedBinary-Pfad). Das ist die „natives Pool-Organ
// vs. faithful Flach-Store-Traversal vs. Default-Spiegel"-Dreiteilung im Kern von #188-4b.
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>,
                             void>,
              "#188-4b-a 3-Wege-Split: BST = Pool-Familie -> KEIN faithful-Traversal (void)");
static_assert(!std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>,
                              void>,
              "#188-4b-a 3-Wege-Split: BST = Pool-Familie -> HAT natives Organ (non-void)");
static_assert(!std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::LinearScanSearchAlgo>,
                              void>,
              "#188-4b-a 3-Wege-Split: linear_scan = Such-METHODE -> HAT faithful-Traversal (non-void)");
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::LinearScanSearchAlgo>, void>,
              "#188-4b-a 3-Wege-Split: linear_scan = Such-METHODE -> KEIN Pool-Organ (void)");
// Gruppe (3): direkt-adressierter Flach-Array-Wrapper ist in BEIDEN Traits void (weder Pool noch faithful-
// Traversal) -> beweist, dass die 9 Pool-Familien eine ECHTE Teilmenge der nicht-store-traversierbaren Menge sind.
static_assert(std::is_same_v<organ_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>, void>,
              "#188-4b-a 3-Wege-Split: Array256 = Flach-Array -> KEIN Pool-Organ (void)");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>, void>,
              "#188-4b-a 3-Wege-Split: Array256 = Flach-Array -> KEIN faithful-Traversal (void, Default-SortedBinary)");

} // namespace composable
} // namespace comdare::cache_engine::lookup
