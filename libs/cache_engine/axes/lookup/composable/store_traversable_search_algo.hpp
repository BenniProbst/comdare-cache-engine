#pragma once
// E-Welle-A2 (Befund-2 / Q2-Schritt-4) — Schritt 1: Klassifikation store-traversierbarer Such-Algorithmen.
//
// @topic traversal @achse 03a @schicht composable
//
// **Frage:** Ist ein Such-Algorithmus über einen FLACHEN Slot-Store (LayoutAwareChunkedStore) traversierbar —
// d.h. die search_algo-Achse kann ÜBER denselben node/layout/allocator-getriebenen Store suchen (Befund-2-SOLL,
// Doc 34 §9) — ODER braucht er ein eigenes Knoten-Pool-Substrat (Tree/Trie/Hash), das NICHT flach-traversierbar ist?
//
// **Autoritative Klassifikations-Grundlage (G3, code-bestätigt via `tier_to_organ_mapping.hpp` #40 +
// `axis_03a_search_algo_registry.hpp`):**
//   • Array-/Flach-Familie → ComposedSearch<Traversal, RawSlotStore> (flach) → STORE-TRAVERSIERBAR.
//     #188-4c-ii: Array256/Array65535 via DirectAddressTraversal, VectorU8U8/VectorU16U16 via SortedVectorTraversal.
//   • Tree/Trie/Hash-Familie → Composed*Search<Traversal, *NodePoolStore> → (noch) NICHT flach-traversierbar
//     (das Pool-Substrat IST der Algorithmus); diese Tiere laufen DERZEIT über search_organ_ (Weg-B).
//
// ⚠️ ARCHITEKTUR-SOLL (Doc 30 §6 Q2 + User-Direktive A10, 2026-06-25, Task #188): die separate
// search_organ_/container_-Aufteilung ist ein BUG, KEIN gewolltes Design — „Q2 ist ein Bug, kein Geschmack"
// (Doc 30:125). Der Soll (Doc 30:111) ist EIN Speicher: `container_` trägt das ECHTE
// `Composition::search_algo` für ALLE Familien, `search_organ_` ENTFÄLLT (Q2 Schritt 4 = OFFEN). Die hiesige
// Weg-B-Klassifikation ist ein TEMPORÄRER Stopgap bis zum container_-ComposedSearch-Substrat-Umbau
// (#188 Inkremente 4a k-ary/Eytzinger-Traversal-Organe · 4b Pool-Familie über node/layout/allocator ·
// 4c search_organ_-Entfernung) — KEINE dauerhafte „ehrliche Limitierung".
//
// **Marker-basiert (KEIN Raten, Meta-Lehre #1/#2):** jeder store-traversierbare search_algo-Wrapper trägt
// `static constexpr bool axis_03a_store_traversable = true;` — je Wrapper am Code verifiziert, NICHT pauschal.
// Fehlt der Marker → konservativ `false` (Weg-B). Die `static_assert`-Verifikation läuft über die ZIEL-Population
// (`EnabledStrategies`), nicht nur über Referenz-Kompositionen.

#include <concepts>
#include <type_traits>

namespace comdare::cache_engine::lookup::composable {

// Detektiert den (am Wrapper verifizierten) Marker; ohne Marker konservativ false.
template <class S>
inline constexpr bool store_traversable_search_algo_v = requires {
    { S::axis_03a_store_traversable } -> std::convertible_to<bool>;
} && [] {
    if constexpr (requires { S::axis_03a_store_traversable; })
        return S::axis_03a_store_traversable;
    else
        return false;
}();

// Concept: S sucht über einen flachen Slot-Store (Array-Familie) → in A2.5 wird container_t-Traversal = das zu S
// passende Array-Traversal-Organ (statt hart-verdrahtetem SortedBinary), search_organ_ entfällt für S.
template <class S>
concept StoreTraversableSearchAlgo = store_traversable_search_algo_v<S>;

} // namespace comdare::cache_engine::lookup::composable
