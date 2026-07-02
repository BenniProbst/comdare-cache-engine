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

#include "composable_search.hpp" // LinearScan/SortedBinary + RawSlotStore + ComposedSearch
#include "interpolation_traversal_organ.hpp"
#include "composed_tree_search.hpp"      // BSTTraversalOrgan + TreeNodePoolStore + ComposedTreeSearch
#include "composed_hash_search.hpp"      // HashProbeTraversalOrgan + HashBucketPoolStore + ComposedHashSearch (#41)
#include "composed_skip_list_search.hpp" // SkipListTraversalOrgan + SkipListNodePoolStore + ComposedSkipListSearch (#41)
#include "composed_btree_search.hpp"     // BTreeTraversalOrgan + BTreeNodePoolStore + ComposedBTreeSearch (#41)
#include "composed_art_trie_search.hpp" // ArtTrieTraversalOrgan + ArtTrieNodePoolStore + ComposedArtTrieSearch (#43 s4)
#include "composed_hot_patricia_search.hpp" // HotPatriciaTraversalOrgan + HotPatriciaNodePoolStore + ComposedHotPatriciaSearch (#43 s4)
#include "composed_wormhole_search.hpp" // WormholeJumpTraversalOrgan + WormholeLeafListPoolStore + ComposedWormholeSearch (#43 s4)
#include "composed_surf_map_search.hpp" // SurfMapTraversalOrgan + SurfFstMapPoolStore + ComposedSurfMapSearch (#43 s4 SuRF-Map-Schale)
#include "composed_start_trie_search.hpp" // StartTrieTraversalOrgan + StartTrieNodePoolStore + ComposedStartTrieSearch (#43 s4)
#include "composed_masstree_search.hpp" // MasstreeLayerTraversalOrgan + MasstreeLayerNodePoolStore + ComposedMasstreeSearch (#42-Folge)
#include "observable_composed_search.hpp"    // ObservableComposedSearch — ObservableAxis-Huelle flache Organe (#42)
#include "observable_composed_container.hpp" // ObservableComposedContainer — ObservableAxis-Huelle Container-Organe (#42)

namespace comdare::cache_engine::lookup::composable {

// --- Organ-Pendants (Stufe-1-Gegenstuecke, uint64-Key) — die Bausteine der Gattungs-Konfiguratoren -------
using LinearScanOrgan    = ComposedSearch<LinearScanTraversal, RawSlotStore>;
using SortedBinaryOrgan  = ComposedSearch<SortedBinaryTraversal, RawSlotStore>;
using InterpolationOrgan = ComposedSearch<InterpolationTraversalOrgan, RawSlotStore>;
template <class Shape>
using BstTreeOrganShaped = ComposedTreeSearch<BSTTraversalOrgan, TreeNodePoolStore<Shape>>;
using BstTreeOrgan       = ComposedTreeSearch<BSTTraversalOrgan, TreeNodePoolStore<>>;
// V41 Umstufung-A (#41) — sezierte CE-native Such-Strukturen als Organ-Kompositionen (eigene Pool-Familien):
template <class Shape>
using HashSearchOrganShaped = ComposedHashSearch<HashProbeTraversalOrgan, HashBucketPoolStore<Shape>>;
using HashSearchOrgan = ComposedHashSearch<HashProbeTraversalOrgan, HashBucketPoolStore<>>;      // HashSearchAlgo S14
template <class Shape>
using SkipListOrganShaped = ComposedSkipListSearch<SkipListTraversalOrgan, SkipListNodePoolStore<Shape>>;
using SkipListOrgan = ComposedSkipListSearch<SkipListTraversalOrgan, SkipListNodePoolStore<>>; // SkipListSearchAlgo S13
template <class Shape>
using BTreeSearchOrganShaped = ComposedBTreeSearch<BTreeTraversalOrgan, BTreeNodePoolStore<Shape>>;
using BTreeSearchOrgan = ComposedBTreeSearch<BTreeTraversalOrgan, BTreeNodePoolStore<>>; // BTreeSearchAlgo S17

// V41 Umstufung-A (#41) — OriginalXxx-Tiere (S04-S08) auf bestehende flache Organe seziert.
// **Befund (Planrunde 2026-05-29):** Die OriginalXxx-Wrapper-BODIES sind triviale C++23-Re-Impls (KEIN
// Paper-Code im Body): ART = std::array<optional,256> (== Array256 == LinearScan); HOT/START/Wormhole/SuRF
// = sorted-vector+lower_bound (== VectorU8U8/U16U16 == SortedBinary). `is_original`/Habich bleibt am Wrapper
// (Bodies unangetastet); das Organ seziert NUR das IST-Such-Verhalten. Die ECHTE Trie-Anatomie (Node4/16/48/
// 256, LOUDS, wormmeta-Hash-Anchor, Patricia) lebt nur im physischen Paper-Code → eigene s4-Charge
// (node_type x path_compression x Trie-Traversal Mehr-Achsen-Modell + extern-C-Linking), NICHT #41.
// S04 ART: ECHTE Trie-Anatomie (#43 s4) — adaptive Node4/16/48/256 + ByteWise-Path-Compression + Byte-Descent.
// (Loest den fruehen flachen LinearScanOrgan-Platzhalter ab: ART ist jetzt als echtes Organ rekonstruiert.)
using ArtTrieOrgan     = ComposedArtTrieSearch<ArtTrieTraversalOrgan, ArtTrieNodePoolStore>;
using OriginalArtOrgan = ArtTrieOrgan;
// S05 HOT: ECHTE bit-level Patricia-Anatomie (#43 s4) — crit-bit (countl_zero MSB-first) + Single-Bit-Split +
// Collapse-Erase. is_original=false ([[pseudocode-papers-fallback]]); Multi-Bit/SparsePartialKeys+SIMD = Folge.
using HotPatriciaOrgan = ComposedHotPatriciaSearch<HotPatriciaTraversalOrgan, HotPatriciaNodePoolStore>;
using OriginalHotOrgan = HotPatriciaOrgan; // S05 HOT (war flacher SortedBinaryOrgan-Platzhalter)
// S06 START: ECHTE Multibyte-Span-Anatomie (#43 s4) — Adaptive Radix Tree mit per-Node-Span (1/2/3-Byte-
// Diskriminator, span-2 = Rewired64K-Distinktion ggue. ARTs fixem 1-Byte) + ByteWise-Path-Compression.
// is_original=false ([[pseudocode-papers-fallback]]; volle Quelle fehlt). Cost-DP-Self-Tuning (adaptive
// Span-Wahl) = Folge-Achse axis_03t_node_tuning. Loest den letzten flachen SortedBinaryOrgan-Platzhalter ab.
using StartTrieOrgan = ComposedStartTrieSearch<StartTrieTraversalOrgan<2>, StartTrieNodePoolStore>; // span-2 Multibyte
using OriginalStartOrgan = StartTrieOrgan; // S06 START (war letzter flacher SortedBinaryOrgan-Platzhalter)
// S07 Wormhole: ECHTE Hybrid-Anatomie (#43 s4) — sortierte doppelt-verkettete Leaf-Liste + Hash-Anchor-Jump
// (groesster Anker<=key statt Wurzel-Abstieg) + Leaf-Split/Merge. is_original=false ([[pseudocode-papers-fallback]];
// wh.c GPL-3.0, KEIN extern-C-Linking). Loest den flachen SortedBinaryOrgan-Platzhalter ab.
using WormholeOrgan         = ComposedWormholeSearch<WormholeJumpTraversalOrgan, WormholeLeafListPoolStore>;
using OriginalWormholeOrgan = WormholeOrgan; // S07 Wormhole (war flacher SortedBinaryOrgan-Platzhalter)
// S08 SuRF: exakte Map-Schale (#43 s4) — autoritatives exaktes K->V (sortiert), traegt std::map-Vergleichbarkeit.
// Das echte LOUDS-Succinct-Range-Filter-Organ (may-contain, bool) lebt separat in axis_filter (Gattungs-Trennung).
// is_original=false ([[pseudocode-papers-fallback]]). Loest den flachen SortedBinaryOrgan-Platzhalter ab.
using SurfMapOrgan      = ComposedSurfMapSearch<SurfMapTraversalOrgan, SurfFstMapPoolStore>;
using OriginalSurfOrgan = SurfMapOrgan; // S08 SuRF (war flacher SortedBinaryOrgan-Platzhalter)

// --- Observable-Organ-Aliase (#42 Umstufung-B Phase 1) — die im search_algo-Slot der Gattungs-Konfiguratoren
// (compositions/*_reference.hpp) gefuehrten OBSERVABLEN Organe. ObservableAxis-Pflicht (snapshot_t=
// SearchAlgoStatistics), damit observe_all() (search_algorithm_anatomy.hpp:68) + der abi_adapter-F15-Workload
// (abi_adapter.hpp:149) echte Per-Achsen-Statistik liefern statt EmptyAxisSnapshot (Saeule-2, Doku 24 §5.2;
// Memory feedback_zwei_dimensionen_messmodell). Container-Organe → ObservableComposedContainer; das flache
// Naeherungs-Organ fuer Masstree → bestehendes ObservableComposedSearch. Reine Typ-Aliase (keine Instantiierung).
using ObservableArtTrieOrgan     = ObservableComposedContainer<ArtTrieOrgan>;
using ObservableHotPatriciaOrgan = ObservableComposedContainer<HotPatriciaOrgan>;
using ObservableWormholeOrgan    = ObservableComposedContainer<WormholeOrgan>;
using ObservableSurfMapOrgan     = ObservableComposedContainer<SurfMapOrgan>;
using ObservableStartTrieOrgan   = ObservableComposedContainer<StartTrieOrgan>;
// Masstree: ECHTES B+Baum-of-Tries-Organ (#42-Folge, Planrunde wxciy2wjk) — Voll-Dekomposition mit kpermuter-
// Knoten (cache-craftiness) + Multi-Layer-Slice-Tries. SliceBytes=2 erzwingt echte Mehr-Layer am uint64-
// Interface (make_new_layer real erreichbar, NICHT toter Code) => Namensanspruch erfuellt ([[algorithm-
// correctness-when-named]]); SliceBytes=8 = Single-Layer-B+Baum-Degenerationsanker. Loest den frueheren flachen
// ObservableSortedBinaryOrgan-Platzhalter ab (letzter Platzhalter-Konfigurator -> echtes Organ).
using MasstreeOrgan           = ComposedMasstreeSearch<MasstreeLayerTraversalOrgan<2>, MasstreeLayerNodePoolStore>;
using ObservableMasstreeOrgan = ObservableComposedContainer<MasstreeOrgan>;

/// Dokumentiertes Tier→Organ-Paar (für den Äquivalenz-/Rekonstruktions-Test konsumierbar).
/// `tier` = monolithischer axis_03a-Wrapper (noch Achsen-Wert, bis zur Umstufung).
/// `organ` = äquivalente Organ-Komposition (das Stufe-1-Gegenstück, das ihn ersetzt).
template <class Tier, class Organ>
struct TierOrganPair {
    using tier  = Tier;
    using organ = Organ;
};

} // namespace comdare::cache_engine::lookup::composable
