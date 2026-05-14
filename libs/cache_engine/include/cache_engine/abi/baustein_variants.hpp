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
// Achse 2: Node-Type (V12.6 mit description + paper_ref)
// ─────────────────────────────────────────────────────────────────────────────
struct NodeSparseNode4Art {
    static constexpr char tag[]         = "SPARSE_NODE4_ART";
    static constexpr char description[] = "ART sparse Node4 with key-array + child-array";
    static constexpr char paper_ref[]   = "P01";
};
struct NodeHotBitNode {
    static constexpr char tag[]         = "HOT_BIT_NODE";
    static constexpr char description[] = "HOT bit-encoded node with rank-select";
    static constexpr char paper_ref[]   = "P02";
};
struct NodeMasstreeTrie {
    static constexpr char tag[]         = "MASSTREE_TRIE";
    static constexpr char description[] = "Masstree trie node with permutation index";
    static constexpr char paper_ref[]   = "P03";
};
struct NodeBPlus {
    static constexpr char tag[]         = "BPLUS_NODE";
    static constexpr char description[] = "Standard B+ tree node (ordered keys + child pointers)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct NodeWormholeHash {
    static constexpr char tag[]         = "WORMHOLE_HASH";
    static constexpr char description[] = "Wormhole hash-table node with prefix sharing";
    static constexpr char paper_ref[]   = "P07";
};
struct NodeSurfTrie {
    static constexpr char tag[]         = "SURF_TRIE";
    static constexpr char description[] = "SuRF succinct trie node (LOUDS encoding)";
    static constexpr char paper_ref[]   = "P10";
};
struct NodePrtArtRedirect {
    static constexpr char tag[]         = "PRTART_REDIRECT";
    static constexpr char description[] = "PRT-ART redirect node with virtual-offset jump table";
    static constexpr char paper_ref[]   = "PRTART";
};
struct NodePrtArtBPlus {
    static constexpr char tag[]         = "PRTART_BPLUS";
    static constexpr char description[] = "PRT-ART B+ leaf node (cache-line-aligned)";
    static constexpr char paper_ref[]   = "PRTART";
};

using NodeAxis = algorithm_axis<
    NodeSparseNode4Art, NodeHotBitNode, NodeMasstreeTrie,
    NodeBPlus, NodeWormholeHash, NodeSurfTrie,
    NodePrtArtRedirect, NodePrtArtBPlus>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 3: Traversal (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct TraversalStandard {
    static constexpr char tag[]         = "STANDARD";
    static constexpr char description[] = "Standard top-down traversal (no special heuristic)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct TraversalHotPath {
    static constexpr char tag[]         = "HOT_PATH";
    static constexpr char description[] = "Hot-path-recognition with adaptive caching of frequent prefixes";
    static constexpr char paper_ref[]   = "P02 / P05";
};
struct TraversalRangeScan {
    static constexpr char tag[]         = "RANGE_SCAN";
    static constexpr char description[] = "Range-scan-optimized leaf traversal (linked-list shortcut)";
    static constexpr char paper_ref[]   = "P06 / P10";
};
struct TraversalRangeHotPath {
    static constexpr char tag[]         = "RANGE_HOT_PATH";
    static constexpr char description[] = "Combined hot-path + range-scan (range queries on cached prefixes)";
    static constexpr char paper_ref[]   = "P25 / P27";
};
struct TraversalPrtArtHotPath {
    static constexpr char tag[]         = "PRTART_HOT_PATH";
    static constexpr char description[] = "PRT-ART hot-path with redirect-prefetch + density-tracker integration";
    static constexpr char paper_ref[]   = "PRTART";
};

using TraversalAxis = algorithm_axis<
    TraversalStandard, TraversalHotPath, TraversalRangeScan,
    TraversalRangeHotPath, TraversalPrtArtHotPath>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 4: Value-Handle (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct ValueHandleInline {
    static constexpr char tag[]         = "INLINE";
    static constexpr char description[] = "Value stored inline in node (small-value-optimization)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct ValueHandleExternal {
    static constexpr char tag[]         = "EXTERNAL";
    static constexpr char description[] = "Value stored externally, node holds pointer";
    static constexpr char paper_ref[]   = "(generic)";
};
struct ValueHandleChainRef {
    static constexpr char tag[]         = "CHAINREF";
    static constexpr char description[] = "Chain-reference handle (linked-list of value-blocks, PRT-ART)";
    static constexpr char paper_ref[]   = "PRTART";
};

using ValueHandleAxis = algorithm_axis<
    ValueHandleInline, ValueHandleExternal, ValueHandleChainRef>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 5: Concurrency (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct ConcurrencySingleThreaded {
    static constexpr char tag[]         = "SINGLE_THREADED";
    static constexpr char description[] = "No synchronization (single-threaded baseline)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct ConcurrencySharedMutex {
    static constexpr char tag[]         = "SHARED_MUTEX";
    static constexpr char description[] = "C++17 std::shared_mutex (single-write, multi-read)";
    static constexpr char paper_ref[]   = "(C++17)";
};
struct ConcurrencyOptimisticLockCoupling {
    static constexpr char tag[]         = "OPTIMISTIC_LOCK_COUPLING";
    static constexpr char description[] = "OLC: per-node version counters with optimistic validation";
    static constexpr char paper_ref[]   = "P08";
};
struct ConcurrencyRcu {
    static constexpr char tag[]         = "RCU";
    static constexpr char description[] = "Read-Copy-Update with grace-period-based reclamation";
    static constexpr char paper_ref[]   = "P29";
};
struct ConcurrencyHazardPointers {
    static constexpr char tag[]         = "HAZARD_POINTERS";
    static constexpr char description[] = "Hazard-pointer-based safe memory reclamation";
    static constexpr char paper_ref[]   = "P30";
};
struct ConcurrencyPrtArtOlcReserved {
    static constexpr char tag[]         = "PRTART_OLC_RESERVED";
    static constexpr char description[] = "PRT-ART OLC variant with reserved value-blocks for atomicity";
    static constexpr char paper_ref[]   = "PRTART";
};

using ConcurrencyAxis = algorithm_axis<
    ConcurrencySingleThreaded, ConcurrencySharedMutex,
    ConcurrencyOptimisticLockCoupling, ConcurrencyRcu,
    ConcurrencyHazardPointers, ConcurrencyPrtArtOlcReserved>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 6: Allocator (V12.6 — 16 Familien aus Allokator_Matrix.txt + paper_ref)
// ─────────────────────────────────────────────────────────────────────────────
struct AllocSystemMalloc {
    static constexpr char tag[] = "SYSTEM_MALLOC";
    static constexpr char description[] = "Default platform malloc (libc / msvcrt)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct AllocHoard {
    static constexpr char tag[] = "HOARD";
    static constexpr char description[] = "Hoard scalable multithreaded allocator";
    static constexpr char paper_ref[]   = "A01";
};
struct AllocSlab {
    static constexpr char tag[] = "SLAB";
    static constexpr char description[] = "Bonwick slab allocator (object-cache)";
    static constexpr char paper_ref[]   = "A02";
};
struct AllocMichaelLockFree {
    static constexpr char tag[] = "MICHAEL_LOCKFREE";
    static constexpr char description[] = "Michael scalable lock-free dynamic memory allocator";
    static constexpr char paper_ref[]   = "A03";
};
struct AllocMimalloc {
    static constexpr char tag[] = "MIMALLOC";
    static constexpr char description[] = "Microsoft mimalloc free-list-sharding";
    static constexpr char paper_ref[]   = "A04";
};
struct AllocJemalloc {
    static constexpr char tag[] = "JEMALLOC";
    static constexpr char description[] = "FreeBSD jemalloc concurrent malloc";
    static constexpr char paper_ref[]   = "A05";
};
struct AllocTcmalloc {
    static constexpr char tag[] = "TCMALLOC";
    static constexpr char description[] = "Google TCMalloc thread-caching malloc";
    static constexpr char paper_ref[]   = "A06";
};
struct AllocSnmalloc {
    static constexpr char tag[] = "SNMALLOC";
    static constexpr char description[] = "Microsoft snmalloc message-passing allocator";
    static constexpr char paper_ref[]   = "A07";
};
struct AllocScalloc {
    static constexpr char tag[] = "SCALLOC";
    static constexpr char description[] = "Salzburg scalloc span-pooling allocator";
    static constexpr char paper_ref[]   = "A08";
};
struct AllocNumalloc {
    static constexpr char tag[] = "NUMALLOC";
    static constexpr char description[] = "NUMA-aware allocator (UMass/Berger)";
    static constexpr char paper_ref[]   = "A09";
};
struct AllocRpmalloc {
    static constexpr char tag[] = "RPMALLOC";
    static constexpr char description[] = "rpmalloc public-domain lock-free thread-caching allocator";
    static constexpr char paper_ref[]   = "A10";
};
struct AllocLrmalloc {
    static constexpr char tag[] = "LRMALLOC";
    static constexpr char description[] = "Modern competitive lock-free allocator (Leite/Rocha 2020)";
    static constexpr char paper_ref[]   = "A11";
};
struct AllocCama {
    static constexpr char tag[] = "CAMA";
    static constexpr char description[] = "Cache-aware predictable memory allocator (RTAS 2011)";
    static constexpr char paper_ref[]   = "A12";
};
struct AllocStarMalloc {
    static constexpr char tag[] = "STARMALLOC";
    static constexpr char description[] = "StarMalloc formally verified concurrent allocator";
    static constexpr char paper_ref[]   = "A13";
};
struct AllocBuddy {
    static constexpr char tag[] = "BUDDY";
    static constexpr char description[] = "Knuth/Knowlton buddy system allocator";
    static constexpr char paper_ref[]   = "A19";
};
struct AllocDlmalloc {
    static constexpr char tag[] = "DLMALLOC";
    static constexpr char description[] = "Doug Lea malloc (classic allocator since 1987)";
    static constexpr char paper_ref[]   = "A20";
};

using AllocatorAxis = algorithm_axis<
    AllocSystemMalloc, AllocHoard, AllocSlab, AllocMichaelLockFree,
    AllocMimalloc, AllocJemalloc, AllocTcmalloc, AllocSnmalloc,
    AllocScalloc, AllocNumalloc, AllocRpmalloc, AllocLrmalloc,
    AllocCama, AllocStarMalloc, AllocBuddy, AllocDlmalloc>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 7: Prefetch (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct PrefetchNone {
    static constexpr char tag[]         = "NONE";
    static constexpr char description[] = "No software prefetching";
    static constexpr char paper_ref[]   = "(baseline)";
};
struct PrefetchAdaptiveDistance {
    static constexpr char tag[]         = "ADAPTIVE_DISTANCE";
    static constexpr char description[] = "Adaptive prefetch distance based on observed miss-rate";
    static constexpr char paper_ref[]   = "P21 / P23";
};
struct PrefetchPathOriented {
    static constexpr char tag[]         = "PATH_ORIENTED";
    static constexpr char description[] = "Path-oriented prefetch following active search trajectory";
    static constexpr char paper_ref[]   = "P25";
};
struct PrefetchFractal {
    static constexpr char tag[]         = "FRACTAL";
    static constexpr char description[] = "Chen fractal prefetching (multi-level prefetch chains)";
    static constexpr char paper_ref[]   = "P22";
};
struct PrefetchHierarchical {
    static constexpr char tag[]         = "HIERARCHICAL";
    static constexpr char description[] = "Hierarchical prefetch (multi-cache-level coordination)";
    static constexpr char paper_ref[]   = "P27";
};
struct PrefetchPrtArtRedirectPrefetch {
    static constexpr char tag[]         = "PRTART_REDIRECT_PREFETCH";
    static constexpr char description[] = "PRT-ART redirect-prefetch following virtual-offset jumps";
    static constexpr char paper_ref[]   = "PRTART";
};

using PrefetchAxis = algorithm_axis<
    PrefetchNone, PrefetchAdaptiveDistance, PrefetchPathOriented,
    PrefetchFractal, PrefetchHierarchical, PrefetchPrtArtRedirectPrefetch>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 8: Telemetry (V12.6 — Kuehn 5 Strategien aus REV 6 INK-4)
// ─────────────────────────────────────────────────────────────────────────────
struct TelemetryOff {
    static constexpr char tag[]         = "OFF";
    static constexpr char description[] = "No telemetry collection (lowest overhead)";
    static constexpr char paper_ref[]   = "(baseline)";
};
struct TelemetryLeafonlySampled {
    static constexpr char tag[]         = "LEAFONLY_SAMPLED";
    static constexpr char description[] = "Sample only leaf-node accesses (low-overhead profiling)";
    static constexpr char paper_ref[]   = "P28";
};
struct TelemetryAllNodes {
    static constexpr char tag[]         = "ALL_NODES";
    static constexpr char description[] = "Full per-node access counters (high-fidelity)";
    static constexpr char paper_ref[]   = "P28";
};
struct TelemetryVampirOtf2 {
    static constexpr char tag[]         = "VAMPIR_OTF2";
    static constexpr char description[] = "VAMPIR OTF2 trace integration (TU Dresden ZIH)";
    static constexpr char paper_ref[]   = "P33";
};
struct TelemetryPikaProfile {
    static constexpr char tag[]         = "PIKA_PROFILE";
    static constexpr char description[] = "PIKA HPC-job profiler integration (TU Dresden ZIH)";
    static constexpr char paper_ref[]   = "(ZIH)";
};

using TelemetryAxis = algorithm_axis<
    TelemetryOff, TelemetryLeafonlySampled, TelemetryAllNodes,
    TelemetryVampirOtf2, TelemetryPikaProfile>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 9: ISA (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct IsaX86Baseline {
    static constexpr char tag[]         = "X86_BASELINE";
    static constexpr char description[] = "x86-64 baseline (no SIMD extensions)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct IsaX86Sse42 {
    static constexpr char tag[]         = "X86_SSE42";
    static constexpr char description[] = "x86-64 with SSE 4.2 (16-byte SIMD)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct IsaX86Avx2 {
    static constexpr char tag[]         = "X86_AVX2";
    static constexpr char description[] = "x86-64 with AVX2 (32-byte SIMD, gather/scatter)";
    static constexpr char paper_ref[]   = "P31";
};
struct IsaX86Avx512 {
    static constexpr char tag[]         = "X86_AVX512";
    static constexpr char description[] = "x86-64 with AVX-512 (64-byte SIMD, mask registers)";
    static constexpr char paper_ref[]   = "P32";
};
struct IsaArmNeon {
    static constexpr char tag[]         = "ARM_NEON";
    static constexpr char description[] = "ARM NEON SIMD (16-byte vectors)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct IsaArmSve {
    static constexpr char tag[]         = "ARM_SVE";
    static constexpr char description[] = "ARM Scalable Vector Extension (variable-length vectors)";
    static constexpr char paper_ref[]   = "(generic)";
};

using IsaAxis = algorithm_axis<
    IsaX86Baseline, IsaX86Sse42, IsaX86Avx2, IsaX86Avx512,
    IsaArmNeon, IsaArmSve>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 10: Layout (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct LayoutStandardHeap {
    static constexpr char tag[]         = "STANDARD_HEAP";
    static constexpr char description[] = "Standard heap allocation (no special arrangement)";
    static constexpr char paper_ref[]   = "(baseline)";
};
struct LayoutCacheObliviousVeb {
    static constexpr char tag[]         = "CACHE_OBLIVIOUS_VEB";
    static constexpr char description[] = "Cache-oblivious van Emde Boas tree layout";
    static constexpr char paper_ref[]   = "P16 / P17";
};
struct LayoutCacheLineAligned {
    static constexpr char tag[]         = "CACHE_LINE_ALIGNED";
    static constexpr char description[] = "Nodes aligned on 64-byte cache-line boundaries";
    static constexpr char paper_ref[]   = "P11 / P12";
};
struct LayoutNumaPinned {
    static constexpr char tag[]         = "NUMA_PINNED";
    static constexpr char description[] = "NUMA-pinned allocation (origin-aware page placement)";
    static constexpr char paper_ref[]   = "A09";
};
struct LayoutHbmHybrid {
    static constexpr char tag[]         = "HBM_HYBRID";
    static constexpr char description[] = "HBM/DRAM hybrid layout (hot pages in HBM, cold in DRAM)";
    static constexpr char paper_ref[]   = "(REV 6 INK-6)";
};
struct LayoutPrtArtMultiLevel {
    static constexpr char tag[]         = "PRTART_MULTI_LEVEL";
    static constexpr char description[] = "PRT-ART multi-level layout with TLB-offset optimization";
    static constexpr char paper_ref[]   = "PRTART";
};

using LayoutAxis = algorithm_axis<
    LayoutStandardHeap, LayoutCacheObliviousVeb, LayoutCacheLineAligned,
    LayoutNumaPinned, LayoutHbmHybrid, LayoutPrtArtMultiLevel>;

// ─────────────────────────────────────────────────────────────────────────────
// Achse 11: Reclamation (V12.6)
// ─────────────────────────────────────────────────────────────────────────────
struct ReclamationNone {
    static constexpr char tag[]         = "NONE";
    static constexpr char description[] = "No safe-memory-reclamation (single-threaded)";
    static constexpr char paper_ref[]   = "(baseline)";
};
struct ReclamationRcuGp {
    static constexpr char tag[]         = "RCU_GP";
    static constexpr char description[] = "RCU grace-period reclamation";
    static constexpr char paper_ref[]   = "P29";
};
struct ReclamationHazardPointers {
    static constexpr char tag[]         = "HAZARD_POINTERS";
    static constexpr char description[] = "Michael hazard-pointer reclamation";
    static constexpr char paper_ref[]   = "P30";
};
struct ReclamationEpochBased {
    static constexpr char tag[]         = "EPOCH_BASED";
    static constexpr char description[] = "Epoch-based reclamation (Fraser/Harris-style)";
    static constexpr char paper_ref[]   = "(generic)";
};
struct ReclamationCrystallineLockFree {
    static constexpr char tag[]         = "CRYSTALLINE_LOCKFREE";
    static constexpr char description[] = "Crystalline family of lock-free wait-free reclamation";
    static constexpr char paper_ref[]   = "A17";
};

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
