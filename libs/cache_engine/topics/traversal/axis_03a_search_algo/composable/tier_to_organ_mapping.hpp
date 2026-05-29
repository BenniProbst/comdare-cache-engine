#pragma once
// V41 #40 (Doku 24 §6.2, Doku 14 §3/§9) — Tier→Organ-REKONSTRUKTIONS-Mapping.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Zweck = Rekonstruktions-Beleg, NICHT „Wrapper behalten":** Dieser Header ordnet jedem bereits in
// Organe SEZIERTEN monolithischen Tier-Wrapper seine **äquivalente Organ-Komposition** zu. Er ist die
// **Brücke** für die eigentliche Umstufung (Doku 24 §6.2): er belegt, dass das Tier exakt aus Organen
// wiederherstellbar ist (Doku 14 §3.3 „genau eine Permutation entspricht dem Original-Algorithmus").
//
// **Erklärte Ordnung (User 2026-05-29, geschärft):** Achsen enthalten NUR Organe — NIE monolithische
// Tiere. Ein Tier darf NUR SEZIERT vorliegen (ausschließlich als Organ-Komposition / Gattungs-Konfigurator);
// ein noch nicht seziertes Tier steht AUSSERHALB des Systems (Doku 14 §3.1) und ist KEIN Achsen-Wert. Sobald
// seziert (= unten gemappt), gehört das Tier als Gattungs-Konfigurator (Composition über Organ-Achsen) unter
// seine Gattung — NICHT als axis_03a-Achsen-Wert. Die tatsächliche Entfernung aus `EnabledStrategies` +
// Rekonstruktion als Composition erfolgt im Umstufungs-Programm (erst restliche Tiere sezieren, dann ALLE
// umstufen — #41 Sezierung, #42 Entfernung). Daher trägt dieser Header BEWUSST KEINE „mp_size muss 17 bleiben
// / kein Wrapper entfernt"-Invariante (die würde die Umstufung blockieren) — die Äquivalenz wird im Test
// belegt, nicht eingefroren.

#include "composable_search.hpp"                  // LinearScan/SortedBinary + RawSlotStore + ComposedSearch
#include "interpolation_traversal_organ.hpp"
#include "composed_tree_search.hpp"               // BSTTraversalOrgan + TreeNodePoolStore + ComposedTreeSearch
#include "composed_hash_search.hpp"               // HashProbeTraversalOrgan + HashBucketPoolStore + ComposedHashSearch (#41)
#include "composed_skip_list_search.hpp"          // SkipListTraversalOrgan + SkipListNodePoolStore + ComposedSkipListSearch (#41)

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

// --- Organ-Pendants (Stufe-1-Gegenstuecke, uint64-Key) — die Bausteine der Gattungs-Konfiguratoren -------
using LinearScanOrgan    = ComposedSearch<LinearScanTraversal,         RawSlotStore>;
using SortedBinaryOrgan  = ComposedSearch<SortedBinaryTraversal,       RawSlotStore>;
using InterpolationOrgan = ComposedSearch<InterpolationTraversalOrgan, RawSlotStore>;
using BstTreeOrgan       = ComposedTreeSearch<BSTTraversalOrgan,       TreeNodePoolStore>;
// V41 Umstufung-A (#41) — sezierte CE-native Such-Strukturen als Organ-Kompositionen (eigene Pool-Familien):
using HashSearchOrgan    = ComposedHashSearch<HashProbeTraversalOrgan, HashBucketPoolStore>;   // HashSearchAlgo S14
using SkipListOrgan      = ComposedSkipListSearch<SkipListTraversalOrgan, SkipListNodePoolStore>; // SkipListSearchAlgo S13

/// Dokumentiertes Tier→Organ-Paar (für den Äquivalenz-/Rekonstruktions-Test konsumierbar).
/// `tier` = monolithischer axis_03a-Wrapper (noch Achsen-Wert, bis zur Umstufung).
/// `organ` = äquivalente Organ-Komposition (das Stufe-1-Gegenstück, das ihn ersetzt).
template <class Tier, class Organ>
struct TierOrganPair {
    using tier  = Tier;
    using organ = Organ;
};

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
