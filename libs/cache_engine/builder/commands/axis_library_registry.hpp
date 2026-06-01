#pragma once
// V32.HH.1 (2026-05-18) + V33.A.4 (2026-05-21) - Lookup-Tabelle pro Achse fuer AutoPermutator
//
// @subsystem CEB
// @phase_owner CEB

#include "auto_permutator.hpp"
#include <vector>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief AxisLibraryRegistry - statische Lookup-Tabelle pro Achse 1-14 (+ Sub-Achsen)
 * @subsystem CEB
 *
 * Wird vom AutoPermutator.discover_axis_implementations() aufgerufen.
 * Liefert pro Achsen-ID die Liste verfuegbarer SOTA-Bausteine.
 *
 * V32.HH.1: 7 Achsen (11, 12.1-12.3, 13.1-13.2, 6.2).
 * V33.A.4: alle 14 Achsen + relevante Sub-Achsen abgedeckt (3.A, 3.B, 3.M, 6.1, 6.3, 6.4, 6.5, 8.1, 8.2, 9, 10, 12.4, 12.5, 13.3, 13.4, 13.5, 14 + Achsen 1, 2, 4, 5, 7).
 * Quelle: comdare-Diplomarbeit/docs/bausteine/01_bausteine_matrix.md.
 *
 * V34+ Folge: Doxygen-Tag-Extraktion via Custom-Script (BB.1 Konvention) ersetzt diese harte Tabelle.
 */
class AxisLibraryRegistry {
public:
    /// Gibt alle verfuegbaren Variants fuer eine Achse zurueck
    [[nodiscard]] static std::vector<AxisVariant> lookup(std::string_view axis_id) {
        std::vector<AxisVariant> result;

        // ===== Achse 1: PAGE-TYPE (26 Bausteine, Auswahl) =====
        if (axis_id == "1") {
            result.push_back({"1", "DenseByte_ART256", "adapters/P01-ART/"});
            result.push_back({"1", "SparseNode_ART4_16_48", "adapters/P01-ART/"});
            result.push_back({"1", "Compound_HOT", "adapters/P02-HOT/"});
            result.push_back({"1", "Multibyte_START", "adapters/P05-START/"});
            result.push_back({"1", "Macro_CoCo", "adapters/P04-CoCo-trie/"});
            result.push_back({"1", "Metatrieht_Wormhole", "adapters/P07-Wormhole/"});
            result.push_back({"1", "LOUDS_Dense_SuRF", "adapters/P10-SuRF/"});
            result.push_back({"1", "BPlus_Masstree", "adapters/P03-Masstree/"});
            result.push_back({"1", "Decision_B2Tree", "adapters/P06-B2tree/"});
            result.push_back({"1", "CSS_Node", "libs/cache_engine/algorithm_profiles/sota/css_tree.profile.xml"});
            result.push_back({"1", "CSB_NodeGroup", "libs/cache_engine/algorithm_profiles/sota/csb_tree.profile.xml"});
            result.push_back({"1", "Adaptive_BTreesAreBack", "adapters/P20-BTreesAreBack/"});

        // ===== Achse 2: NODE-TYPE (13 Bausteine, Auswahl) =====
        } else if (axis_id == "2") {
            result.push_back({"2", "ART_Node4", "adapters/P01-ART/"});
            result.push_back({"2", "ART_Node16", "adapters/P01-ART/"});
            result.push_back({"2", "ART_Node48", "adapters/P01-ART/"});
            result.push_back({"2", "ART_Node256", "adapters/P01-ART/"});
            result.push_back({"2", "HOT_Compound_K32", "adapters/P02-HOT/"});
            result.push_back({"2", "HOT_Binode", "adapters/P02-HOT/"});
            result.push_back({"2", "Masstree_Internal", "adapters/P03-Masstree/"});
            result.push_back({"2", "Masstree_Border", "adapters/P03-Masstree/"});
            result.push_back({"2", "B2Tree_Decision", "adapters/P06-B2tree/"});
            result.push_back({"2", "B2Tree_Span", "adapters/P06-B2tree/"});

        // ===== Achse 3.A: Algorithm-Seite Traversal =====
        } else if (axis_id == "3.A") {
            result.push_back({"3.A", "ByteByByte", "adapters/P01-ART/"});
            result.push_back({"3.A", "DiscriminativeBits", "adapters/P02-HOT/"});
            result.push_back({"3.A", "LayerSlice", "adapters/P03-Masstree/"});
            result.push_back({"3.A", "MacroNode", "adapters/P04-CoCo-trie/"});
            result.push_back({"3.A", "MultibyteSpan", "adapters/P05-START/"});
            result.push_back({"3.A", "EmbeddedDecTree", "adapters/P06-B2tree/"});
            result.push_back({"3.A", "BPlusBinarySearch", "libs/cache_engine/algorithm_profiles/sota/csb_tree.profile.xml"});

        // ===== Achse 3.B: Cache-Seite Traversal =====
        } else if (axis_id == "3.B") {
            result.push_back({"3.B", "CacheLineLinear", "cache_engine/concepts/traversal/cache_line_linear.hpp"});
            result.push_back({"3.B", "PrefetchPipelined", "adapters/P21-PrefetchingBPlus/"});
            result.push_back({"3.B", "OffsetBasedJump", "prt_art/include/prt_art/default_lookup/prt_art_3b_cache_traversal_default.hpp"});

        // ===== Achse 3.M: Mapping (Virtual Offset Calculator) =====
        } else if (axis_id == "3.M") {
            result.push_back({"3.M", "DirectIndex", "cache_engine/concepts/traversal/direct_index.hpp"});
            result.push_back({"3.M", "VirtualOffsetCalc", "prt_art/include/prt_art/traversal/traversal_mapping.hpp"});
            result.push_back({"3.M", "HashedSlot", "cache_engine/concepts/traversal/hashed_slot.hpp"});

        // ===== Achse 4: VALUEHANDLE (5 Bausteine) =====
        } else if (axis_id == "4") {
            result.push_back({"4", "Inline", "prt_art/include/prt_art/value_handle/inline_handle.hpp"});
            result.push_back({"4", "External", "prt_art/include/prt_art/value_handle/external_handle.hpp"});
            result.push_back({"4", "ChainRef", "prt_art/include/prt_art/value_handle/chain_ref_handle.hpp"});
            result.push_back({"4", "Tagged_4Bit", "cache_engine/concepts/value/tagged_4bit.hpp"});
            result.push_back({"4", "OffsetEncoded", "cache_engine/concepts/value/offset_encoded.hpp"});

        // ===== Achse 5: MEMORY-LAYOUT (8 Bausteine) =====
        } else if (axis_id == "5") {
            result.push_back({"5", "CacheLineAligned", "cache_engine/concepts/layout/cache_line_aligned.hpp"});
            result.push_back({"5", "TLBOffsetVirtual", "prt_art/include/prt_art/memory_layout/tlb_offset.hpp"});
            result.push_back({"5", "vEBLayout", "cache_engine/concepts/layout/veb_layout.hpp"});
            result.push_back({"5", "FractalLayout", "cache_engine/concepts/layout/fractal_layout.hpp"});
            result.push_back({"5", "BFS_Flat", "cache_engine/concepts/layout/bfs_flat.hpp"});
            result.push_back({"5", "DFS_Flat", "cache_engine/concepts/layout/dfs_flat.hpp"});

        // ===== Achse 6.1: Allocator-Family =====
        } else if (axis_id == "6.1") {
            result.push_back({"6.1", "Hoard", "adapters/A01-hoard/"});
            result.push_back({"6.1", "MichaelLockfree", "adapters/A03-michael-lockfree/"});
            result.push_back({"6.1", "mimalloc", "adapters/A04-mimalloc/"});
            result.push_back({"6.1", "jemalloc", "adapters/A05-jemalloc/"});
            result.push_back({"6.1", "tcmalloc", "adapters/A06-tcmalloc/"});
            result.push_back({"6.1", "snmalloc", "adapters/A07-snmalloc/"});
            result.push_back({"6.1", "scalloc", "adapters/A08-scalloc/"});
            result.push_back({"6.1", "rpmalloc", "adapters/A10-rpmalloc/"});
            result.push_back({"6.1", "lrmalloc", "adapters/A11-lrmalloc/"});
            result.push_back({"6.1", "dlmalloc", "adapters/A20-dlmalloc/"});

        // ===== Achse 6.2 (existiert schon): Reclamation =====
        } else if (axis_id == "6.2") {
            result.push_back({"6.2", "EpochBased", "cache_engine/concepts/mechanics/comdare_rcu_mechanic.hpp"});
            result.push_back({"6.2", "RCU", "cache_engine/reclamation/rcu_reclaim/"});
            result.push_back({"6.2", "HazardPointer", "adapters/P30-HazardPointers/"});
            result.push_back({"6.2", "QSBR", "cache_engine/concepts/mechanics/qsbr_mechanic.hpp"});

        // ===== Achse 6.3: NUMA Affinity =====
        } else if (axis_id == "6.3") {
            result.push_back({"6.3", "Local", "cache_engine/concepts/numa_affinity.hpp::NumaAffinity::Local"});
            result.push_back({"6.3", "Interleave", "cache_engine/concepts/numa_affinity.hpp::NumaAffinity::Interleave"});
            result.push_back({"6.3", "Preferred", "cache_engine/concepts/numa_affinity.hpp::NumaAffinity::Preferred"});
            result.push_back({"6.3", "Bind", "cache_engine/concepts/numa_affinity.hpp::NumaAffinity::Bind"});

        // ===== Achse 6.4: Huge-Page Strategy =====
        } else if (axis_id == "6.4") {
            result.push_back({"6.4", "NoHugePages", "cache_engine/concepts/huge_page/none.hpp"});
            result.push_back({"6.4", "Transparent2MB", "cache_engine/concepts/huge_page/transparent_2mb.hpp"});
            result.push_back({"6.4", "Explicit2MB", "cache_engine/concepts/huge_page/explicit_2mb.hpp"});
            result.push_back({"6.4", "Explicit1GB", "cache_engine/concepts/huge_page/explicit_1gb.hpp"});

        // ===== Achse 6.5: Allocator-Pool-Count =====
        } else if (axis_id == "6.5") {
            result.push_back({"6.5", "SinglePool", "single global pool"});
            result.push_back({"6.5", "PrtArt_4Plus2", "prt-art: A/B/C/D + R + V-static/V-dynamic"});
            result.push_back({"6.5", "ThreadLocal", "per-thread arena"});
            result.push_back({"6.5", "PerCorePool", "per-CPU-core arena (NUMA-aware)"});

        // ===== Achse 7: PREFETCH (6 Bausteine) =====
        } else if (axis_id == "7") {
            result.push_back({"7", "None", "no prefetch"});
            result.push_back({"7", "DistanceEstimator", "prt_art/include/prt_art/prefetch/distance_estimator.hpp"});
            result.push_back({"7", "PathOriented", "prt_art/include/prt_art/prefetch/path_oriented.hpp"});
            result.push_back({"7", "RedirectPrefetch", "prt_art/include/prt_art/prefetch/redirect_prefetch.hpp"});
            result.push_back({"7", "HierarchicalBundle", "adapters/P27-hp-soft/"});
            result.push_back({"7", "FractalChen", "libs/cache_engine/algorithm_profiles/sota/chen_fractal.profile.xml"});

        // ===== Achse 8.1: Concurrency-Pattern =====
        } else if (axis_id == "8.1") {
            result.push_back({"8.1", "OptimisticLockCoupling", "adapters/P08-ART-Sync/"});
            result.push_back({"8.1", "MultiReaderSingleWriter", "cache_engine/concurrency_manager/mrsw/"});
            result.push_back({"8.1", "ReadWriteLock", "cache_engine/concurrency_manager/rw_lock/"});
            result.push_back({"8.1", "LockFreeCAS", "cache_engine/concurrency_manager/lockfree/"});
            result.push_back({"8.1", "Snapshot", "cache_engine/concurrency_manager/snapshot/"});
            result.push_back({"8.1", "SingleThreaded", "no synchronization"});
            result.push_back({"8.1", "Transactional", "cache_engine/concurrency_manager/tx/"});

        // ===== Achse 8.2: Locking-Mode =====
        } else if (axis_id == "8.2") {
            result.push_back({"8.2", "Optimistic", "cache_engine/concepts/locking_mode.hpp::LockingMode::Optimistic"});
            result.push_back({"8.2", "Pessimistic", "cache_engine/concepts/locking_mode.hpp::LockingMode::Pessimistic"});
            result.push_back({"8.2", "LockFree", "cache_engine/concepts/locking_mode.hpp::LockingMode::LockFree"});
            result.push_back({"8.2", "WaitFree", "cache_engine/concepts/locking_mode.hpp::LockingMode::WaitFree"});

        // ===== Achse 9: ISA-Targeting =====
        } else if (axis_id == "9") {
            result.push_back({"9", "Generic", "no SIMD intrinsics"});
            result.push_back({"9", "SSE4_2", "cache_engine/concepts/isa/sse42.hpp"});
            result.push_back({"9", "AVX2", "cache_engine/concepts/isa/avx2.hpp"});
            result.push_back({"9", "AVX512F", "cache_engine/concepts/isa/avx512f.hpp"});
            result.push_back({"9", "ARM_NEON", "cache_engine/concepts/isa/arm_neon.hpp"});
            result.push_back({"9", "ARM_SVE2", "cache_engine/concepts/isa/arm_sve2.hpp"});

        // ===== Achse 10: MEASUREMENT-DETAIL =====
        } else if (axis_id == "10") {
            result.push_back({"10", "Throughput_Only", "cache_engine/concepts/measurement/throughput.hpp"});
            result.push_back({"10", "Latency_p50_p99", "cache_engine/concepts/measurement/latency.hpp"});
            result.push_back({"10", "CacheMissCounters", "cache_engine/concepts/measurement/cache_miss.hpp"});
            result.push_back({"10", "Full_F1_Matrix", "libs/test_infra/measurement_matrix/"});

        // ===== Achse 11 (existiert schon): TELEMETRY =====
        } else if (axis_id == "11") {
            result.push_back({"11", "LeafOnlyCounter", "cache_engine/concepts/telemetry/leaf_only_counter.hpp"});
            result.push_back({"11", "LeafOnlySampledCounter", "cache_engine/concepts/telemetry/leaf_only_sampled_counter.hpp"});
            result.push_back({"11", "RetroactiveAggregator", "cache_engine/concepts/telemetry/retroactive_aggregation.hpp"});
            result.push_back({"11", "PathReadCounter", "cache_engine/concepts/telemetry/path_read_counter.hpp"});

        // ===== Achse 12.1 (existiert schon): SIMD-Family =====
        } else if (axis_id == "12.1") {
            result.push_back({"12.1", "Scalar", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::Scalar"});
            result.push_back({"12.1", "AVX2", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::AVX2"});
            result.push_back({"12.1", "AVX512", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::AVX512"});
            result.push_back({"12.1", "NEON", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::NEON"});
            result.push_back({"12.1", "SVE2", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::SVE2"});

        } else if (axis_id == "12.2") {
            result.push_back({"12.2", "L1Aware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::L1Aware"});
            result.push_back({"12.2", "L2Aware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::L2Aware"});
            result.push_back({"12.2", "L3Aware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::L3Aware"});
            result.push_back({"12.2", "HBMAware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::HBMAware"});

        } else if (axis_id == "12.3") {
            result.push_back({"12.3", "Local", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Local"});
            result.push_back({"12.3", "Interleave", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Interleave"});
            result.push_back({"12.3", "Preferred", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Preferred"});
            result.push_back({"12.3", "Bind", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Bind"});

        // ===== Achse 12.4: Prefetch-Distance =====
        } else if (axis_id == "12.4") {
            result.push_back({"12.4", "NoPrefetch", "cache_engine/concepts/hardware/no_prefetch.hpp"});
            result.push_back({"12.4", "PrefetchT0", "_mm_prefetch(_MM_HINT_T0)"});
            result.push_back({"12.4", "PrefetchT1", "_mm_prefetch(_MM_HINT_T1)"});
            result.push_back({"12.4", "PrefetchT2", "_mm_prefetch(_MM_HINT_T2)"});
            result.push_back({"12.4", "PrefetchNTA", "_mm_prefetch(_MM_HINT_NTA)"});

        // ===== Achse 12.5: Atomic-Granularity =====
        } else if (axis_id == "12.5") {
            result.push_back({"12.5", "CAS_64", "atomic_compare_exchange_64"});
            result.push_back({"12.5", "CAS_128", "cmpxchg16b / casp"});
            result.push_back({"12.5", "LL_SC", "load-linked/store-conditional (ARM)"});
            result.push_back({"12.5", "Relaxed_Ordering", "memory_order_relaxed counters"});

        // ===== Achse 13.1 (existiert schon): Worker-Pool-Layout =====
        } else if (axis_id == "13.1") {
            result.push_back({"13.1", "ThreadPerCore", "cache_engine/concepts/scheduling_strategy.hpp::WorkerPoolLayout::ThreadPerCore"});
            result.push_back({"13.1", "WorkStealing", "cache_engine/concepts/scheduling_strategy.hpp::WorkerPoolLayout::WorkStealing"});
            result.push_back({"13.1", "CpuPinning", "cache_engine/concepts/scheduling_strategy.hpp::WorkerPoolLayout::CpuPinning"});

        } else if (axis_id == "13.2") {
            result.push_back({"13.2", "Limit_1", "1 SIMD-Worker"});
            result.push_back({"13.2", "Limit_2", "2 SIMD-Worker (Default, Hardware-Limit)"});
            result.push_back({"13.2", "Limit_4", "4 SIMD-Worker"});

        // ===== Achse 13.3: Heterogeneous-Awareness =====
        } else if (axis_id == "13.3") {
            result.push_back({"13.3", "Homogeneous", "alle Worker auf gleichen Cores"});
            result.push_back({"13.3", "HybridAware", "Performance- vs Efficiency-Cores beruecksichtigen"});

        // ===== Achse 13.4: Memory-Interleave =====
        } else if (axis_id == "13.4") {
            result.push_back({"13.4", "NoInterleave", "single NUMA node"});
            result.push_back({"13.4", "RoundRobin", "page-level round-robin"});
            result.push_back({"13.4", "FirstTouch", "kernel default"});

        // ===== Achse 13.5: Batching =====
        } else if (axis_id == "13.5") {
            result.push_back({"13.5", "Single", "no batching"});
            result.push_back({"13.5", "MicroBatch", "4-16 ops per batch"});
            result.push_back({"13.5", "MacroBatch", "128+ ops per batch"});

        // ===== Achse 14: ENGINE-CHOICE =====
        } else if (axis_id == "14") {
            result.push_back({"14", "V1_Manual_Profile", "AlgorithmProfile gewaehlt manuell"});
            result.push_back({"14", "V2_Adaptive_RuntimeDetect", "ICacheStrategy waehlt anhand Workload"});
            result.push_back({"14", "V3_Hybrid_AB", "halb manuell + halb adaptive"});
            result.push_back({"14", "V4_Automatic_FullPermutation", "CEB enumeriert alle V32-Permutationen"});

        // ===== Achse 15.1 V35.B: Compiler-Family =====
        } else if (axis_id == "15.1") {
            result.push_back({"15.1", "GCC",        "GNU GCC 9+ (Linux primary, Windows via MinGW)"});
            result.push_back({"15.1", "Clang",      "LLVM Clang 13+ (Linux/Windows via clang-cl)"});
            result.push_back({"15.1", "AppleClang", "Apple Clang 14+ (macOS Xcode-toolchain)"});
            result.push_back({"15.1", "MSVC",       "MSVC 14.30+ cl.exe (Windows primary)"});

        // ===== Achse 15.2 V35.B: Optimization-Level =====
        } else if (axis_id == "15.2") {
            result.push_back({"15.2", "O0",     "no optimization (Debug)"});
            result.push_back({"15.2", "O1",     "basic optimization"});
            result.push_back({"15.2", "O2",     "standard optimization (Release-Default)"});
            result.push_back({"15.2", "O3",     "aggressive optimization (vectorization, inlining)"});
            result.push_back({"15.2", "Ofast",  "O3 + unsafe-math (POSIX)"});
            result.push_back({"15.2", "MSVC_Od","MSVC /Od (kein Optimieren)"});
            result.push_back({"15.2", "MSVC_O1","MSVC /O1 (Groessen-Optimieren)"});
            result.push_back({"15.2", "MSVC_O2","MSVC /O2 (Geschwindigkeit, Release-Default)"});

        // ===== Achse 15.3 V35.B: LTO-Mode =====
        } else if (axis_id == "15.3") {
            result.push_back({"15.3", "None",       "no link-time optimization"});
            result.push_back({"15.3", "ThinLTO",    "Clang/GCC -flto=thin (parallelisierbar)"});
            result.push_back({"15.3", "FullLTO",    "Clang/GCC -flto (single-thread, bestere Performance)"});
            result.push_back({"15.3", "MSVC_LTCG",  "MSVC /GL + /LTCG (link-time code generation)"});

        // ===== Achse 15.4 V35.B: PGO-Profile =====
        } else if (axis_id == "15.4") {
            result.push_back({"15.4", "None",          "kein Profile-Guided-Optimization"});
            result.push_back({"15.4", "Generate",      "GCC/Clang -fprofile-generate / MSVC /GENPROFILE"});
            result.push_back({"15.4", "Use",           "GCC/Clang -fprofile-use / MSVC /USEPROFILE"});
            result.push_back({"15.4", "SamplePGO",     "Clang AutoFDO via perf samples (no instrumentation)"});

        // ===== Achse 15.5 V35.B: Target-Arch =====
        } else if (axis_id == "15.5") {
            result.push_back({"15.5", "native",     "-march=native (Host-CPU max)"});
            result.push_back({"15.5", "x86-64-v3",  "AVX2 baseline (Haswell+, Excavator+)"});
            result.push_back({"15.5", "x86-64-v4",  "AVX-512 baseline (Skylake-X+, Sapphire Rapids)"});
            result.push_back({"15.5", "znver4",     "Zen 4 (AMD Ryzen 7000+/EPYC 9004)"});
            result.push_back({"15.5", "armv9-a",    "ARMv9 (Apple M-Series, Cortex-X3+)"});
            result.push_back({"15.5", "generic",    "-mtune=generic, no -march"});
        }

        return result;
    }
};

// AutoPermutator::discover_axis_implementations() Implementation (HH.1)
inline void AutoPermutator::discover_axis_implementations() {
    available_variants_ = AxisLibraryRegistry::lookup(axis_id_);
}

}  // namespace comdare::cache_engine::builder::commands
