#pragma once
// 234-V-a (2026-07-07, User-GO Option A) -- 2-armige Shaped-Erweiterung der Tier->Organ-Naht.
//
// @topic traversal @achse 03a @schicht composable
//
// **Zweck:** `organ_for_search_algo_shaped<S, Shape>` waehlt fuer eine organ-backed Pool-Familie das
// SHAPED-Organ (z.B. `BTreeSearchOrganShaped<BtreeOrderKt8>`), wenn ein echter Shape-Traeger anliegt.
// Die bestehende EINARMIGE Naht `organ_for_search_algo<S>` bleibt UNANGETASTET (golden-neutral, Z.198):
// dieser Header ist ein reiner Sibling; Konsument ist der ABI-Adapter ueber seinen defaulted
// ShapeCarrier-Parameter (void = exakt das einarmige Verhalten, typ-identisch).
//
// **Disziplin (Z.198 / Doc 30):** Shape ist KEIN 20. Slot -- die 19-Slot-ABI-Invariante bleibt; die
// Scheinmultiplikation familienfremder Kombinationen wird ZWEIFACH verhindert: (1) auf Typ-Ebene faellt
// jede nicht spezialisierte Familie auf die einarmige Zuordnung zurueck (Shape wirkt nicht), (2) der
// Emitter emittiert Shaped-Quellen NUR hinter dem organ_for!=void-Filter (adhoc_emitter.hpp).
//
// **234-V-b (2026-07-08):** BST/Hash/SkipList ergaenzt; SwissTable + ART/HOT/START/Wormhole/SuRF/
// Eytzinger/Masstree bleiben mangels Shape-Param out of scope.

#include "organ_for_search_algo.hpp" // einarmige Naht + Familien-Fwd-Decls + Shaped-Aliase (via tier_to_organ_mapping.hpp)

namespace comdare::cache_engine::lookup::composable {

// Primaer: Shape wirkt nicht -> exakt die einarmige Zuordnung. Deckt (a) Shape=void fuer ALLE Familien
// (Level-0-Neutralitaet) und (b) familienfremde (S, Shape)-Kombinationen (kein Organ-Wechsel, keine
// Scheinmultiplikation auf Typ-Ebene).
template <class S, class Shape>
struct organ_for_search_algo_shaped {
    using type = organ_for_search_algo_t<S>;
};

// BTree (S17) -- 234-V-a Beweis-Familie: ein echter Shape-Traeger (BtreeOrderKtN) waehlt das Shaped-Organ.
template <class Shape>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BTreeSearchAlgo, Shape> {
    using type = BTreeSearchOrganShaped<Shape>;
};

// Level-0 explizit: ohne Shape-Traeger bleibt der einarmige Default (== Kt4-Anker, test_234_f1).
template <>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BTreeSearchAlgo, void> {
    using type = organ_for_search_algo_t<::comdare::cache_engine::lookup::BTreeSearchAlgo>;
};

// BST (S16) -- echter Shape-Traeger waehlt das BST-Shaped-Organ.
template <class Shape>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo, Shape> {
    using type = BstTreeOrganShaped<Shape>;
};

// Level-0 explizit: void bleibt typ-identisch zur einarmigen BST-Zuordnung.
template <>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo, void> {
    using type = organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>;
};

// Hash (S14) -- echter Shape-Traeger waehlt das Hash-Shaped-Organ.
template <class Shape>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::HashSearchAlgo, Shape> {
    using type = HashSearchOrganShaped<Shape>;
};

// Level-0 explizit: void bleibt typ-identisch zur einarmigen Hash-Zuordnung.
template <>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::HashSearchAlgo, void> {
    using type = organ_for_search_algo_t<::comdare::cache_engine::lookup::HashSearchAlgo>;
};

// SkipList (S13) -- echter Shape-Traeger waehlt das SkipList-Shaped-Organ.
template <class Shape>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::SkipListSearchAlgo, Shape> {
    using type = SkipListOrganShaped<Shape>;
};

// Level-0 explizit: void bleibt typ-identisch zur einarmigen SkipList-Zuordnung.
template <>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::SkipListSearchAlgo, void> {
    using type = organ_for_search_algo_t<::comdare::cache_engine::lookup::SkipListSearchAlgo>;
};

template <class S, class Shape>
using organ_for_search_algo_shaped_t = typename organ_for_search_algo_shaped<S, Shape>::type;

// Self-proving (kein Raten): void-Neutralitaet -- der Shaped-Trait ist mit Shape=void fuer die Beweis-
// Familie UND eine fremde Familie typ-identisch zur einarmigen Naht (=> Adapter-Default byte-identisch).
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::BTreeSearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::BTreeSearchAlgo>>,
              "234-V-a: Level-0 (void) muss exakt die einarmige BTree-Zuordnung liefern");
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::Array256SearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>>,
              "234-V-a: fremde Familie (flach, organ_for=void) bleibt unter dem Shaped-Trait void");
static_assert(
    std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo, void>,
                   organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>>,
    "234-V-b: Level-0 (void) muss exakt die einarmige BST-Zuordnung liefern");
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::HashSearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::HashSearchAlgo>>,
              "234-V-b: Level-0 (void) muss exakt die einarmige Hash-Zuordnung liefern");
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::SkipListSearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::SkipListSearchAlgo>>,
              "234-V-b: Level-0 (void) muss exakt die einarmige SkipList-Zuordnung liefern");

} // namespace comdare::cache_engine::lookup::composable
