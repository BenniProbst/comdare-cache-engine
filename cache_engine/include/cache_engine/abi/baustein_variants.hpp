#pragma once
// baustein_variants.hpp — Konkrete std::variant Konkretisierungen pro 11 Achsen (REV 7.6 V9.2)
//
// Operationalisierung des Concept-Patterns aus algorithm_baustein.hpp.
// Pro Achse ein std::variant ueber alle SOTA-Konkretisierungen + PRT-ART.
//
// Vorgehensweise:
// - Tag-basiertes Skelett (eine struct pro Konkretisierung als Marker).
// - Folge-Phase ersetzt die Tag-Strukturen durch echte Konkretisierungs-Klassen
//   mit Body (Page-Layout, Node-Algo, etc.).
// - Variants kompilieren sich vollstaendig — koennen vom CacheEngineBuilder
//   benutzt werden, um Cartesian-Product-Permutationen zu generieren.

#include "algorithm_baustein.hpp"

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::baustein {

// REV 7.6 V11.4 — Konkrete Variant-Bodies mit Identitaets-Konstanten:
// Pro Tag-Struktur: tag + description + paper_ref (Compile-Time-Reflection).

// ─────────────────────────────────────────────────────────────────────────────
// Achse 1: Page-Layout
// ─────────────────────────────────────────────────────────────────────────────
struct PageStandardHeap {
    static constexpr char tag[]         = "STANDARD_HEAP";
    static constexpr char description[] = "Default heap-allocated page (no special layout)";
    static constexpr char paper_ref[]   = "(none)";
};
struct PageDenseByteArt256 {
    static constexpr char tag[]         = "DENSEBYTE_ART256";
    static constexpr char description[] = "ART Node256 dense byte page (Leis 2013)";
    static constexpr char paper_ref[]   = "P01";
};
struct PageHotMultiByte {
    static constexpr char tag[]         = "HOT_MULTIBYTE";
    static constexpr char description[] = "HOT multi-byte page with bit-vector indices (Binna 2018)";
    static constexpr char paper_ref[]   = "P02";
};
struct PageMasstreeINode {
    static constexpr char tag[]         = "MASSTREE_INODE";
    static constexpr char description[] = "Masstree internal node with permutation array (Mao 2012)";
    static constexpr char paper_ref[]   = "P03";
};
struct PageCoCoCompact {
    static constexpr char tag[]         = "COCO_COMPACT";
    static constexpr char description[] = "CoCo-Trie compact page with succinct encoding (Boffa 2024)";
    static constexpr char paper_ref[]   = "P04";
};
struct PageStart {
    static constexpr char tag[]         = "START_PAGE";
    static constexpr char description[] = "START self-tuning ART page (Fent 2020)";
    static constexpr char paper_ref[]   = "P05";
};
struct PageB2Tree {
    static constexpr char tag[]         = "B2TREE_PAGE";
    static constexpr char description[] = "B-squared Tree page with two-level fanout (Schmeisser 2022)";
    static constexpr char paper_ref[]   = "P06";
};
struct PageWormhole {
    static constexpr char tag[]         = "WORMHOLE_PAGE";
    static constexpr char description[] = "Wormhole page with hash-array hybrid (Wu 2019)";
    static constexpr char paper_ref[]   = "P07";
};
struct PageSurf {
    static constexpr char tag[]         = "SURF_PAGE";
    static constexpr char description[] = "SuRF succinct range filter page (Zhang 2018)";
    static constexpr char paper_ref[]   = "P10";
};
struct PagePrtArtDenseByte {
    static constexpr char tag[]         = "PRTART_DENSEBYTE";
    static constexpr char description[] = "PRT-ART dense byte page (Pruefling)";
    static constexpr char paper_ref[]   = "PRTART";
};
struct PagePrtArtRedirect {
    static constexpr char tag[]         = "PRTART_REDIRECT";
    static constexpr char description[] = "PRT-ART redirect page with virtual-offset jump (Pruefling)";
    static constexpr char paper_ref[]   = "PRTART";
};

using PageAxis = algorithm_axis<
    PageStandardHeap, PageDenseByteArt256, PageHotMultiByte,
    PageMasstreeINode, PageCoCoCompact, PageStart, PageB2Tree,
    PageWormhole, PageSurf, PagePrtArtDenseByte, PagePrtArtRedirect>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 2: Node-Type
// ─────────────────────────────────────────────────────────────────────────────
struct NodeSparseNode4Art      { static constexpr char tag[] = "SPARSE_NODE4_ART"; };
struct NodeHotBitNode          { static constexpr char tag[] = "HOT_BIT_NODE"; };
struct NodeMasstreeTrie        { static constexpr char tag[] = "MASSTREE_TRIE"; };
struct NodeBPlus               { static constexpr char tag[] = "BPLUS_NODE"; };
struct NodeWormholeHash        { static constexpr char tag[] = "WORMHOLE_HASH"; };
struct NodeSurfTrie            { static constexpr char tag[] = "SURF_TRIE"; };
struct NodePrtArtRedirect      { static constexpr char tag[] = "PRTART_REDIRECT"; };
struct NodePrtArtBPlus         { static constexpr char tag[] = "PRTART_BPLUS"; };

using NodeAxis = algorithm_axis<
    NodeSparseNode4Art, NodeHotBitNode, NodeMasstreeTrie,
    NodeBPlus, NodeWormholeHash, NodeSurfTrie,
    NodePrtArtRedirect, NodePrtArtBPlus>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 3: Traversal
// ─────────────────────────────────────────────────────────────────────────────
struct TraversalStandard       { static constexpr char tag[] = "STANDARD"; };
struct TraversalHotPath        { static constexpr char tag[] = "HOT_PATH"; };
struct TraversalRangeScan      { static constexpr char tag[] = "RANGE_SCAN"; };
struct TraversalRangeHotPath   { static constexpr char tag[] = "RANGE_HOT_PATH"; };
struct TraversalPrtArtHotPath  { static constexpr char tag[] = "PRTART_HOT_PATH"; };

using TraversalAxis = algorithm_axis<
    TraversalStandard, TraversalHotPath, TraversalRangeScan,
    TraversalRangeHotPath, TraversalPrtArtHotPath>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 4: Value-Handle
// ─────────────────────────────────────────────────────────────────────────────
struct ValueHandleInline       { static constexpr char tag[] = "INLINE"; };
struct ValueHandleExternal     { static constexpr char tag[] = "EXTERNAL"; };
struct ValueHandleChainRef     { static constexpr char tag[] = "CHAINREF"; };

using ValueHandleAxis = algorithm_axis<
    ValueHandleInline, ValueHandleExternal, ValueHandleChainRef>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 5: Concurrency
// ─────────────────────────────────────────────────────────────────────────────
struct ConcurrencySingleThreaded   { static constexpr char tag[] = "SINGLE_THREADED"; };
struct ConcurrencySharedMutex      { static constexpr char tag[] = "SHARED_MUTEX"; };
struct ConcurrencyOptimisticLockCoupling { static constexpr char tag[] = "OPTIMISTIC_LOCK_COUPLING"; };
struct ConcurrencyRcu              { static constexpr char tag[] = "RCU"; };
struct ConcurrencyHazardPointers   { static constexpr char tag[] = "HAZARD_POINTERS"; };
struct ConcurrencyPrtArtOlcReserved { static constexpr char tag[] = "PRTART_OLC_RESERVED"; };

using ConcurrencyAxis = algorithm_axis<
    ConcurrencySingleThreaded, ConcurrencySharedMutex,
    ConcurrencyOptimisticLockCoupling, ConcurrencyRcu,
    ConcurrencyHazardPointers, ConcurrencyPrtArtOlcReserved>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 6: Allocator (16 Familien aus Allokator_Matrix.txt)
// ─────────────────────────────────────────────────────────────────────────────
struct AllocSystemMalloc       { static constexpr char tag[] = "SYSTEM_MALLOC"; };
struct AllocHoard              { static constexpr char tag[] = "HOARD"; };
struct AllocSlab               { static constexpr char tag[] = "SLAB"; };
struct AllocMichaelLockFree    { static constexpr char tag[] = "MICHAEL_LOCKFREE"; };
struct AllocMimalloc           { static constexpr char tag[] = "MIMALLOC"; };
struct AllocJemalloc           { static constexpr char tag[] = "JEMALLOC"; };
struct AllocTcmalloc           { static constexpr char tag[] = "TCMALLOC"; };
struct AllocSnmalloc           { static constexpr char tag[] = "SNMALLOC"; };
struct AllocScalloc            { static constexpr char tag[] = "SCALLOC"; };
struct AllocNumalloc           { static constexpr char tag[] = "NUMALLOC"; };
struct AllocRpmalloc           { static constexpr char tag[] = "RPMALLOC"; };
struct AllocLrmalloc           { static constexpr char tag[] = "LRMALLOC"; };
struct AllocCama               { static constexpr char tag[] = "CAMA"; };
struct AllocStarMalloc         { static constexpr char tag[] = "STARMALLOC"; };
struct AllocBuddy              { static constexpr char tag[] = "BUDDY"; };
struct AllocDlmalloc           { static constexpr char tag[] = "DLMALLOC"; };

using AllocatorAxis = algorithm_axis<
    AllocSystemMalloc, AllocHoard, AllocSlab, AllocMichaelLockFree,
    AllocMimalloc, AllocJemalloc, AllocTcmalloc, AllocSnmalloc,
    AllocScalloc, AllocNumalloc, AllocRpmalloc, AllocLrmalloc,
    AllocCama, AllocStarMalloc, AllocBuddy, AllocDlmalloc>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 7: Prefetch
// ─────────────────────────────────────────────────────────────────────────────
struct PrefetchNone                  { static constexpr char tag[] = "NONE"; };
struct PrefetchAdaptiveDistance      { static constexpr char tag[] = "ADAPTIVE_DISTANCE"; };
struct PrefetchPathOriented          { static constexpr char tag[] = "PATH_ORIENTED"; };
struct PrefetchFractal               { static constexpr char tag[] = "FRACTAL"; };
struct PrefetchHierarchical          { static constexpr char tag[] = "HIERARCHICAL"; };
struct PrefetchPrtArtRedirectPrefetch { static constexpr char tag[] = "PRTART_REDIRECT_PREFETCH"; };

using PrefetchAxis = algorithm_axis<
    PrefetchNone, PrefetchAdaptiveDistance, PrefetchPathOriented,
    PrefetchFractal, PrefetchHierarchical, PrefetchPrtArtRedirectPrefetch>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 8: Telemetry
// ─────────────────────────────────────────────────────────────────────────────
struct TelemetryOff               { static constexpr char tag[] = "OFF"; };
struct TelemetryLeafonlySampled   { static constexpr char tag[] = "LEAFONLY_SAMPLED"; };
struct TelemetryAllNodes          { static constexpr char tag[] = "ALL_NODES"; };
struct TelemetryVampirOtf2        { static constexpr char tag[] = "VAMPIR_OTF2"; };
struct TelemetryPikaProfile       { static constexpr char tag[] = "PIKA_PROFILE"; };

using TelemetryAxis = algorithm_axis<
    TelemetryOff, TelemetryLeafonlySampled, TelemetryAllNodes,
    TelemetryVampirOtf2, TelemetryPikaProfile>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 9: ISA
// ─────────────────────────────────────────────────────────────────────────────
struct IsaX86Baseline       { static constexpr char tag[] = "X86_BASELINE"; };
struct IsaX86Sse42          { static constexpr char tag[] = "X86_SSE42"; };
struct IsaX86Avx2           { static constexpr char tag[] = "X86_AVX2"; };
struct IsaX86Avx512         { static constexpr char tag[] = "X86_AVX512"; };
struct IsaArmNeon           { static constexpr char tag[] = "ARM_NEON"; };
struct IsaArmSve            { static constexpr char tag[] = "ARM_SVE"; };

using IsaAxis = algorithm_axis<
    IsaX86Baseline, IsaX86Sse42, IsaX86Avx2, IsaX86Avx512,
    IsaArmNeon, IsaArmSve>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 10: Layout
// ─────────────────────────────────────────────────────────────────────────────
struct LayoutStandardHeap        { static constexpr char tag[] = "STANDARD_HEAP"; };
struct LayoutCacheObliviousVeb   { static constexpr char tag[] = "CACHE_OBLIVIOUS_VEB"; };
struct LayoutCacheLineAligned    { static constexpr char tag[] = "CACHE_LINE_ALIGNED"; };
struct LayoutNumaPinned          { static constexpr char tag[] = "NUMA_PINNED"; };
struct LayoutHbmHybrid           { static constexpr char tag[] = "HBM_HYBRID"; };
struct LayoutPrtArtMultiLevel    { static constexpr char tag[] = "PRTART_MULTI_LEVEL"; };

using LayoutAxis = algorithm_axis<
    LayoutStandardHeap, LayoutCacheObliviousVeb, LayoutCacheLineAligned,
    LayoutNumaPinned, LayoutHbmHybrid, LayoutPrtArtMultiLevel>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 11: Reclamation
// ─────────────────────────────────────────────────────────────────────────────
struct ReclamationNone                { static constexpr char tag[] = "NONE"; };
struct ReclamationRcuGp               { static constexpr char tag[] = "RCU_GP"; };
struct ReclamationHazardPointers      { static constexpr char tag[] = "HAZARD_POINTERS"; };
struct ReclamationEpochBased          { static constexpr char tag[] = "EPOCH_BASED"; };
struct ReclamationCrystallineLockFree { static constexpr char tag[] = "CRYSTALLINE_LOCKFREE"; };

using ReclamationAxis = algorithm_axis<
    ReclamationNone, ReclamationRcuGp, ReclamationHazardPointers,
    ReclamationEpochBased, ReclamationCrystallineLockFree>;

// ─────────────────────────────────────────────────────────────────────────────
// Komplette 11-Achsen-Permutation als Default-Type-Alias
// ─────────────────────────────────────────────────────────────────────────────
using DefaultElevenAxes = eleven_axes_permutation<
    PageAxis, NodeAxis, TraversalAxis, ValueHandleAxis,
    ConcurrencyAxis, AllocatorAxis, PrefetchAxis, TelemetryAxis,
    IsaAxis, LayoutAxis, ReclamationAxis>;

// Total moegliche Permutationen (gesamtes Cartesian-Product):
//   11 * 8 * 5 * 3 * 6 * 16 * 6 * 5 * 6 * 6 * 5 = 5,485,363,200
// (Habich-Direktive: full mode generiert _alle_ — nur defined-mode laeuft als
//  vorgewaehlter Subset)

}  // namespace comdare::cache_engine::baustein
