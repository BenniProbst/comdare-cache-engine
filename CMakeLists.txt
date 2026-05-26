# comdare-cache-engine — Top-Level CMakeLists.txt
# Active Cache-Aware Hardware Adaptation Cache Engine for Trie-Based Index Structures
# TU Dresden Diplomarbeit · BEP Venture UG / Marke Comdare · Apache 2.0
#
# Phase 4.B Skelett — KEINE Implementation, nur Build-Struktur

cmake_minimum_required(VERSION 3.28)  # C++23 Modules-Support
project(comdare-cache-engine
        VERSION 0.1.0
        DESCRIPTION "Active Cache-Aware Hardware Adaptation Cache Engine"
        LANGUAGES CXX)

# ─────────────────────────────────────────────────────────────────────────────
# C++ Standard (F-EXTRA-1 — Hauptcompiler IMMER C++23)
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────────────────────────────────────────────────────────
# Build-Modi (F9 — Compile-Time vs Runtime SIMD-Detection, mutually exclusive)
# ─────────────────────────────────────────────────────────────────────────────
set(COMDARE_DETECTION_MODE "COMPILE_TIME" CACHE STRING
    "ISA-Detection: COMPILE_TIME oder RUNTIME (mutually exclusive)")
set_property(CACHE COMDARE_DETECTION_MODE PROPERTY STRINGS COMPILE_TIME RUNTIME)

# ─────────────────────────────────────────────────────────────────────────────
# Build-Optionen (F11 — Mikrobenchmarks)
# ─────────────────────────────────────────────────────────────────────────────
option(COMDARE_BUILD_BUILDER       "Build CacheEngineBuilder (main.cpp)"       ON)
option(COMDARE_BUILD_BENCHMARKS    "Build benchmark-Targets (YCSB + Custom)"   ON)
option(COMDARE_BUILD_TESTS         "Build Google-Tests (generic + specific)"   ON)
option(COMDARE_BUILD_PERMUTATIONS  "Pre-Build aller Modul-Permutationen"       OFF)
option(COMDARE_MICROBENCH_CONTINUOUS "Continuous Mikrobenchmarks (Builder-Modus)" OFF)

# V41.F.6.1.A (2026-05-25) — Statistics/Mess-Overhead per CMake-Flag steuern
# Default ON: alle Topic+Achsen-Implementierungen enthalten statistics() + reset()
# als Pflicht-API (Mess-Reihen + Welch-Test brauchen das).
# Wenn OFF: statistics()+reset()+Statistik-Member werden komplett aus dem Binary
# herauspraepariert (#ifdef'd in den Achsen-Headers). Production-Export einer
# Final-Permutation OHNE Mess-Overhead. CacheEnginePermutationStrategy-Concept
# verlangt statistics()/reset() dann nicht mehr.
option(COMDARE_CE_ENABLE_STATISTICS
    "V41.F.6.1.A: Topic+Achsen-Statistics enabled (jede Achse muss statistics()+reset() bieten)"
    ON)
if(COMDARE_CE_ENABLE_STATISTICS)
    add_compile_definitions(COMDARE_CE_ENABLE_STATISTICS=1)
endif()

# V41.F.6.1.C Stufe 1 (2026-05-25): Zentrale Vendor-Aktivierungs-Flags pro Topic-Achse
# (W6-Pattern: konfigurierte Vendor in mp_list akkumuliert, Compiler verwirft Rest)
# Default ON wenn Vendor-Source im ext/ vorhanden; spaeter dynamisch durch
# CacheEngineBuilder-App ueberschreibbar (siehe §15.7)
option(COMDARE_AXIS_06_ENABLE_STD       "Allocator A22 std::malloc/free (libc)"      ON)
option(COMDARE_AXIS_06_ENABLE_MIMALLOC  "Allocator A04 Mimalloc Free-List-Sharding"  ON)
option(COMDARE_AXIS_06_ENABLE_SNMALLOC  "Allocator A07 Snmalloc Message-Passing"     ON)
option(COMDARE_AXIS_06_ENABLE_PMR       "Allocator A22 std::pmr::memory_resource"    ON)
# V41.F.6.1 Batch 2 Vendor (2026-05-26): Jemalloc / TCMalloc / dlmalloc
option(COMDARE_AXIS_06_ENABLE_JEMALLOC  "Allocator A05 Jemalloc Arena-based (Evans)" ON)
option(COMDARE_AXIS_06_ENABLE_TCMALLOC  "Allocator A06 TCMalloc Thread-Caching (Google)" ON)
option(COMDARE_AXIS_06_ENABLE_DLMALLOC  "Allocator A20 dlmalloc Bins (Doug Lea)"     ON)
# V41.F.6.1 Batch 3 Vendor (2026-05-26): Hoard / Slab / Michael LockFree
option(COMDARE_AXIS_06_ENABLE_HOARD      "Allocator A01 Hoard Per-Heap (Berger/McKinley 2000)" ON)
option(COMDARE_AXIS_06_ENABLE_SLAB       "Allocator A02 Slab Magazine-Cache (Bonwick 1994)"    ON)
option(COMDARE_AXIS_06_ENABLE_MICHAEL_LF "Allocator A03 Michael Lock-Free CAS (PODC 2002)"     ON)
# V41.F.6.1 Batch 4 Vendor (2026-05-26): Scalloc / NUMAlloc / RPMalloc
option(COMDARE_AXIS_06_ENABLE_SCALLOC    "Allocator A08 Scalloc Spans (Aigner PPoPP 2015)"     ON)
option(COMDARE_AXIS_06_ENABLE_NUMALLOC   "Allocator A09 NUMAlloc NUMA-aware (Linden 2018)"     ON)
option(COMDARE_AXIS_06_ENABLE_RPMALLOC   "Allocator A10 RPMalloc Per-Thread-Spans (Jansson 2017)" ON)
# V41.F.6.1 Batch 5 Vendor (2026-05-26): LRMalloc / CAMA / StarMalloc
option(COMDARE_AXIS_06_ENABLE_LRMALLOC   "Allocator A11 LRMalloc Lock-Free + Hazard-Pointer (Leite/Rocha 2019)" ON)
option(COMDARE_AXIS_06_ENABLE_CAMA       "Allocator A12 CAMA Cache-Aware (Bhattacharyya 2020)"    ON)
option(COMDARE_AXIS_06_ENABLE_STARMALLOC "Allocator A13 StarMalloc Formal Verified (Dang/Charguer 2024)" ON)
# V41.F.6.1 Batch 6 Vendor (2026-05-26): TCMalloc-Warehouse / HMalloc / PIM-Malloc
option(COMDARE_AXIS_06_ENABLE_TCMALLOC_WH "Allocator A14 TCMalloc-Warehouse Hyperscale (Google 2020+)" ON)
option(COMDARE_AXIS_06_ENABLE_HMALLOC     "Allocator A15 HMalloc Hybrid Free-List (Tang 2020)"        ON)
option(COMDARE_AXIS_06_ENABLE_PIM_MALLOC  "Allocator A16 PIM-Malloc Processing-In-Memory (UPMEM 2023)" ON)
# V41.F.6.1 Batch 7 Vendor (2026-05-26): Crystalline / Exgen-Malloc / Buddy
option(COMDARE_AXIS_06_ENABLE_CRYSTALLINE "Allocator A17 Crystalline Wait-Free Reclamation (Solodkyy 2021)" ON)
option(COMDARE_AXIS_06_ENABLE_EXGEN       "Allocator A18 Exgen-Malloc Exception-Generated Single-Thread"     ON)
option(COMDARE_AXIS_06_ENABLE_BUDDY       "Allocator A19 Buddy Power-of-2 Splitting (Knuth Klassik)"        ON)
# V41.F.6.1 Batch 8 Vendor (2026-05-26): ptmalloc2 / Vmem-Magazines — Achsen-Vollausbau Abschluss
option(COMDARE_AXIS_06_ENABLE_PTMALLOC2   "Allocator A21 ptmalloc2 explizit (Wolfram Gloger, glibc-Default)" ON)
option(COMDARE_AXIS_06_ENABLE_VMEM_MAG    "Allocator A23 Vmem-Magazines (Bonwick USENIX 2001)"               ON)

# V41.F.6.1 Topic queuing Pilot (2026-05-26): 2 Achsen — axis_Q1 buffer_strategy + axis_Q2 flush_policy
# axis_Q1 Pilot Batch 1 (4 von 13 W2-Strategien)
option(COMDARE_AXIS_Q1_ENABLE_NO_BUFFER    "Buffer-Strategy Q01 NoBuffer (Passthrough)"        ON)
option(COMDARE_AXIS_Q1_ENABLE_FIFO         "Buffer-Strategy Q03 FIFOQueue (Ring/Deque)"        ON)
option(COMDARE_AXIS_Q1_ENABLE_LIFO         "Buffer-Strategy Q04 LIFOStack"                     ON)
option(COMDARE_AXIS_Q1_ENABLE_BOUNDED_RING "Buffer-Strategy Q05 BoundedRing<N> (Disruptor LMAX 2011)" ON)
# Batch 2 (2026-05-26): Q-APP AppendOnly (LSM-MemTable) + Q-PRIO PriorityHeap (Hot-Key)
option(COMDARE_AXIS_Q1_ENABLE_APPEND_ONLY   "Buffer-Strategy Q02 AppendOnly (LSM-MemTable, Bw-Tree Delta)" ON)
option(COMDARE_AXIS_Q1_ENABLE_PRIORITY_HEAP "Buffer-Strategy Q06 PriorityHeap (LRU-Approx, Hot-Key Promo)" ON)
# Batch 3 (2026-05-26): Q-DELTA DeltaChain (Bw-Tree) + Q-SKIP SkiplistBuffer (LSM RocksDB) + Q-TOMB TombstoneBuffer (MVCC)
option(COMDARE_AXIS_Q1_ENABLE_DELTA_CHAIN      "Buffer-Strategy Q07 DeltaChain (Bw-Tree, Levandoski ICDE 2013, versioned)"     ON)
option(COMDARE_AXIS_Q1_ENABLE_SKIPLIST_BUFFER  "Buffer-Strategy Q08 SkiplistBuffer (LSM, RocksDB/LevelDB, ordered)"              ON)
option(COMDARE_AXIS_Q1_ENABLE_TOMBSTONE_BUFFER "Buffer-Strategy Q09 TombstoneBuffer (LSM + MVCC + ART-Optimistik, versioned)"    ON)
# Batch 4 (2026-05-26): Q-COW CopyOnWrite (Persistent ART) + Q-EPOCH EpochBuffer (SMART ART OSDI 2023, iterable_aspect_t) + Q-BATCH BatchedInsertBuffer (OLAP)
option(COMDARE_AXIS_Q1_ENABLE_COPY_ON_WRITE         "Buffer-Strategy Q10 CopyOnWrite (Persistent ART, RCU-Tries, versioned)"              ON)
option(COMDARE_AXIS_Q1_ENABLE_EPOCH_BUFFER          "Buffer-Strategy Q11 EpochBuffer (SMART ART OSDI 2023, Masstree, QSBR, iterable_aspect_t epoch_threshold)" ON)
option(COMDARE_AXIS_Q1_ENABLE_BATCHED_INSERT_BUFFER "Buffer-Strategy Q12 BatchedInsertBuffer (OLAP-Index, ART-Bulk-Insert, batched)"       ON)
# Batch 5 VOLLAUSBAU (2026-05-26): Q-SPSC LockFreeSPSC (Lamport 1983) + Q-MPMC LockFreeMPMC (Vyukov bounded)
option(COMDARE_AXIS_Q1_ENABLE_LOCKFREE_SPSC "Buffer-Strategy Q13a LockFreeSPSC (Lamport 1983, atomic head/tail, lock-free, iterable_aspect_t)" ON)
option(COMDARE_AXIS_Q1_ENABLE_LOCKFREE_MPMC "Buffer-Strategy Q13b LockFreeMPMC (Vyukov bounded MPMC, per-Cell-Sequence, lock-free concurrent)" ON)
# axis_Q2 Pilot Batch 1 (3 von 5 W3-Flush-Policies)
option(COMDARE_AXIS_Q2_ENABLE_EAGER        "Flush-Policy F01 EagerFlush (per-op)"              ON)
option(COMDARE_AXIS_Q2_ENABLE_WATERMARK    "Flush-Policy F02 WatermarkFlush (threshold)"       ON)
option(COMDARE_AXIS_Q2_ENABLE_LAZY         "Flush-Policy F04 LazyFlush (defer)"                ON)
# axis_Q2 Vollausbau Batch 2 (2026-05-26): F03 TimedFlush + F05 AdaptiveLsmFlush
option(COMDARE_AXIS_Q2_ENABLE_TIMED        "Flush-Policy F03 TimedFlush (FS3 time_window, iterable_aspect_t window_ms)" ON)
option(COMDARE_AXIS_Q2_ENABLE_ADAPTIVE_LSM "Flush-Policy F05 AdaptiveLsmFlush (FS4 lernt aus Workload, LSM-spezifisch)"   ON)

# V41.F.6.1 Topic traversal Pilot (2026-05-26): 3 Achsen — axis_03a search_algo + axis_03b cache_traversal + axis_03m mapping
# axis_03a Pilot Batch 1 (3 Strategien analog prt-art Legacy Interpreter)
option(COMDARE_AXIS_03A_ENABLE_ARRAY256       "Search-Algo S01 Array256 (ART Node256 dense, Leis ICDE 2013, SA1 dense, SIMD)" ON)
option(COMDARE_AXIS_03A_ENABLE_VECTOR_U8U8    "Search-Algo S02 VectorU8U8 (HOT Patricia sparse, Binna PVLDB 2018, SA2 sparse, SIMD)" ON)
option(COMDARE_AXIS_03A_ENABLE_VECTOR_U16U16  "Search-Algo S03 VectorU16U16 (START multi-byte Cost-DP, Mertens ICDE 2024, SA3 multilevel)" ON)
# axis_03b Pilot Batch 1 (2 Strategien)
option(COMDARE_AXIS_03B_ENABLE_LINEAR_FANOUT  "Cache-Traversal CT01 LinearFanout (prt-art IFanout linear-scan, CT1 linear)" ON)
option(COMDARE_AXIS_03B_ENABLE_HASH_LOOKUP    "Cache-Traversal CT02 HashLookup (Fibonacci-Hash open-addressing, CT2 hash)" ON)
# axis_03m Pilot Batch 1 (2 Strategien)
option(COMDARE_AXIS_03M_ENABLE_DIRECT_PLACEMENT "Mapping MP01 DirectPlacement (prt-art INode::placement_page_, MP1 direct)" ON)
option(COMDARE_AXIS_03M_ENABLE_POOL_RELATIVE    "Mapping MP02 PoolRelative (prt-art CustomAlignedStructure, MP2 pool_relative)" ON)

# HINWEIS: configure_file fuer axis_06_allocator_flags.hpp wird NACH add_subdirectory(ext)
# am Ende von CMakeLists.txt aufgerufen — dann sind COMDARE_HAVE_<VENDOR> Detection-Flags
# aus ext/CMakeLists.txt bekannt und USE_<VENDOR> = ENABLE && HAVE berechenbar.

# REV 7.6 V8.4 — Experiment-Modus (User-Direktive 2026-05-13/14)
# Aktiviert ResultAggregator + Mess-Hooks in der ExecutionEngine.
# Default OFF: Production-Pfad (kein Mess-Overhead).
# Default ON wird vom Diplomarbeits-messung_driver beim ExperimentDriver-
# Sub-Build ueberschrieben (-DCOMDARE_EXPERIMENT_MODE=ON).
option(COMDARE_EXPERIMENT_MODE
    "Aktiviert ResultAggregator + Mess-Hooks in der ExecutionEngine (REV 7.6 V8.4)"
    OFF)
if(COMDARE_EXPERIMENT_MODE)
    add_compile_definitions(COMDARE_EXPERIMENT_MODE_ON=1)
endif()

# REV 7.6 V8.2 — Legacy-Reimpl-Migration (User-Direktive: PRT-ART gehoert in prt-art)
# V41.struct (2026-05-25): Verzeichnis prt_art/ KOMPLETT geloescht (war leeres
# Skelett, blieb von V23.E-Migration uebrig). Verhindert zirkulare Wahrnehmung
# (comdare-prt-art ist eigenstaendiges Wurzelmodul). Option bleibt fuer
# Cluster-Kompatibilitaet erhalten, hat aber keinen Code mehr.
option(COMDARE_BUILD_LEGACY_REIMPL
    "[REMOVED] PRT-ART legacy_reimpl ist komplett in comdare-prt-art migriert"
    OFF)

# REV 7.6 V13.4 — Stage-1/Stage-2 CMake-Integration
# Stage 1: cache-engine-Builder selbst wird im normalen Build-Stack gebaut.
# Stage 2: Nach Builder-Build wird Builder optional ausgefuehrt um
# vorkonfigurierte Permutations-Module zu generieren.
# Default OFF: Builder-Run wird nur explizit oder vom Diplomarbeits-
# messung_driver getriggert (kein Auto-Run bei Tests/CI).
option(COMDARE_AUTO_RUN_BUILDER
    "Stage 2 (V13.4): nach Builder-Build automatisch Builder ausfuehren"
    OFF)
set(COMDARE_BUILDER_CONFIG_DIR
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/builder/example_configs"
    CACHE PATH "Stage 2 (V13.4): Konfig-Dir fuer Auto-Run des Builders")
set(COMDARE_BUILDER_OUTPUT_DIR
    "${CMAKE_BINARY_DIR}/builder_runs/auto"
    CACHE PATH "Stage 2 (V13.4): Output-Dir fuer Auto-Run des Builders")

# ─────────────────────────────────────────────────────────────────────────────
# Multi-OS-Infrastruktur (REV 6 Migration aus Diplomarbeit/code, 2026-05-12)
# ─────────────────────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(platform_detection)
include(compiler_flags)
include(isa_features)
include(boost_mp11_setup)   # V41.F.6.1.A: Boost.MP11 fuer Permutations-Metaprog.
if(COMDARE_BUILD_TESTS)
  include(gtest_setup)
endif()

# Build-Status-Reporting (Plattform + Block-AO-Erkennung)
message(STATUS "==============================================================")
message(STATUS "comdare-cache-engine Multi-OS-Konfiguration (Phase 6)")
message(STATUS "  OS                : ${COMDARE_OS}")
message(STATUS "  Architektur       : ${COMDARE_ARCH}")
message(STATUS "  Block AO Plattform: ${COMDARE_BLOCK_AO_PLATFORM}")
message(STATUS "  Cache-Line-Size   : ${COMDARE_CACHE_LINE_SIZE} B")
message(STATUS "  ISA detected      : ${COMDARE_ISA_SUMMARY}")
message(STATUS "==============================================================")

# ─────────────────────────────────────────────────────────────────────────────
# Submodule-Check (Korrektur F12-K — comdare-Modules als feste Submodules)
# ─────────────────────────────────────────────────────────────────────────────
include(cmake/check_submodules.cmake OPTIONAL)

# ─────────────────────────────────────────────────────────────────────────────
# V41.F.6.1.P2.A Paper-Legacy-Code Build-Infrastruktur (4 Module)
# Funktionen werden erst aktiv bei Aufruf (P2.B mimalloc Pilot, P2.D Roll-out)
# ─────────────────────────────────────────────────────────────────────────────
include(cmake/is_original_codegen.cmake OPTIONAL)
include(cmake/compiler_cache.cmake     OPTIONAL)
include(cmake/paper_binary.cmake       OPTIONAL)
include(cmake/tools_cache.cmake        OPTIONAL)

# ─────────────────────────────────────────────────────────────────────────────
# Permutations-Codegen (F-EXTRA-5 — Python-Codegen erzeugt CMake-Targets)
# ─────────────────────────────────────────────────────────────────────────────
if(COMDARE_BUILD_PERMUTATIONS)
  include(cmake/permutations.cmake OPTIONAL)
  # generiert build/generated/permutations.cmake (siehe tools/permutation_codegen/)
  # V40.A FIX: ext/ MUSS VOR der Permutations-Inklusion gebaut werden, weil
  # die Permutationen comdare::vendor_mimalloc/jemalloc Targets benoetigen.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/CMakeLists.txt")
    add_subdirectory(ext)
    set(_comdare_ext_added TRUE)
  endif()
  if(EXISTS "${CMAKE_BINARY_DIR}/generated/permutations.cmake")
    include("${CMAKE_BINARY_DIR}/generated/permutations.cmake")
  endif()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Subdirectories (Domaenen 1-6)
# ─────────────────────────────────────────────────────────────────────────────
# V23.C.4 (2026-05-14): measurement migriert nach libs/common/measurement/
# V23.C.3 (2026-05-14): hardware_isa migriert nach libs/common/platform/
# V30.D.2 (2026-05-14): search_engine/ -> libs/search_engine/
add_subdirectory(libs/search_engine)      # Domaene 1
# V30.D.4+D.5 (2026-05-14): cache_engine/ -> libs/cache_engine/ (inkl. builder/)
add_subdirectory(libs/cache_engine)       # Domaene 2 (nutzt 1, 3, 4) + 22 Builder-Subkomponenten
# V30.D.3 (2026-05-14): engine_choice/ -> libs/common/config/ (via libs/common)
# V23.E (2026-05-14): prt_art -> libs/deprecated/prt_art_legacy/
#   (Memory-Direktive: PRT_ART konsumiert CacheEngine, nicht umgekehrt.
#    Echter Code im comdare-prt-art Repo.)
add_subdirectory(adapters)           # Habich-Direktive Adapter
if(NOT _comdare_ext_added)
    add_subdirectory(ext)            # Originalcode (mit Original-Compilern)
endif()
# V23.F (2026-05-14): test_data_accumulation, benchmark_suite, workload_generator
# migriert nach libs/test_infra/
# V30.D.1 (2026-05-14): experiment/ -> libs/execution_engine/ (kein domain/-Zwischenebene)
add_subdirectory(libs/execution_engine)   # Phase 7.3+7.4 (REV 7 §9 Experiment-Loop)
add_subdirectory(libs/common)             # REV 7.7 V23.C (2026-05-14): Cross-Cutting (succinct, ...)
add_subdirectory(libs/test_infra)         # REV 7.7 V23.F (2026-05-14): workload_generator, test_data, benchmark_suite
add_subdirectory(libs/deprecated)         # REV 7.7 V23.E (2026-05-14): Folly-Stil obsoleter Code
add_subdirectory(tools)                   # Phase C (ycsb_cli, 2026-05-13)
add_subdirectory(apps)                    # REV 7.7 V23.B (2026-05-14): Top-Level Executables

if(COMDARE_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(COMDARE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)  # tests/CMakeLists.txt ruft seinerseits add_subdirectory(unit) (REV 6)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Status-Ausgabe
# ─────────────────────────────────────────────────────────────────────────────
message(STATUS "──────────────────────────────────────────────────────────")
message(STATUS "comdare-cache-engine ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:           ${CMAKE_CXX_STANDARD}")
message(STATUS "  Detection Mode:         ${COMDARE_DETECTION_MODE}")
message(STATUS "  Build Builder:          ${COMDARE_BUILD_BUILDER}")
message(STATUS "  Build Benchmarks:       ${COMDARE_BUILD_BENCHMARKS}")
message(STATUS "  Build Tests:            ${COMDARE_BUILD_TESTS}")
message(STATUS "  Pre-Build Permutations: ${COMDARE_BUILD_PERMUTATIONS}")
message(STATUS "  Experiment Mode (V8.4): ${COMDARE_EXPERIMENT_MODE}")
message(STATUS "  Legacy-Reimpl (V8.2):   ${COMDARE_BUILD_LEGACY_REIMPL} [DEPRECATED]")
message(STATUS "  Auto-Run Builder V13.4: ${COMDARE_AUTO_RUN_BUILDER}")
message(STATUS "──────────────────────────────────────────────────────────")

# REV 7.6 V13.4 — Stage 2: Auto-Run des Builders nach Build
# Wird nur im ALL-Target ausgefuehrt wenn COMDARE_AUTO_RUN_BUILDER=ON.
# Erfordert dass der Builder-Target (cache-engine-builder) bereits gebaut wurde.
if(COMDARE_AUTO_RUN_BUILDER AND COMDARE_BUILD_BUILDER)
    add_custom_target(comdare_run_builder ALL
        COMMAND $<TARGET_FILE:comdare-cache-engine-builder>
                "${COMDARE_BUILDER_CONFIG_DIR}"
                "${COMDARE_BUILDER_OUTPUT_DIR}"
                --enumerate-only
                "--comdare-root=${CMAKE_SOURCE_DIR}"
        DEPENDS comdare-cache-engine-builder
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "[V13.4 Stage-2] Auto-Run CacheEngineBuilder (--enumerate-only)"
        VERBATIM)
endif()

# V41.F.6.1.C Stufe 2 (2026-05-25): configure_file fuer axis_06_allocator_flags.hpp
# NACH add_subdirectory(ext) — damit COMDARE_HAVE_<VENDOR> Detection bekannt ist.
# USE_<VENDOR> = ENABLE && HAVE (effektive Aktivierung beruecksichtigt Vendor-Verfuegbarkeit)

# STD: immer verfuegbar (libc malloc)
if(COMDARE_AXIS_06_ENABLE_STD)
    set(COMDARE_AXIS_06_USE_STD 1)
else()
    set(COMDARE_AXIS_06_USE_STD 0)
endif()

# MIMALLOC: ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_MIMALLOC AND COMDARE_HAVE_MIMALLOC)
    set(COMDARE_AXIS_06_USE_MIMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_MIMALLOC 0)
endif()

# SNMALLOC: ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_SNMALLOC AND COMDARE_HAVE_SNMALLOC)
    set(COMDARE_AXIS_06_USE_SNMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_SNMALLOC 0)
endif()

# PMR: immer verfuegbar (C++17 Standard)
if(COMDARE_AXIS_06_ENABLE_PMR)
    set(COMDARE_AXIS_06_USE_PMR 1)
else()
    set(COMDARE_AXIS_06_USE_PMR 0)
endif()

# V41.F.6.1 Batch 2 Vendor — USE = ENABLE && HAVE
# JEMALLOC: ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_JEMALLOC AND COMDARE_HAVE_JEMALLOC)
    set(COMDARE_AXIS_06_USE_JEMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_JEMALLOC 0)
endif()

# TCMALLOC: ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_TCMALLOC AND COMDARE_HAVE_TCMALLOC)
    set(COMDARE_AXIS_06_USE_TCMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_TCMALLOC 0)
endif()

# DLMALLOC: ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_DLMALLOC AND COMDARE_HAVE_DLMALLOC)
    set(COMDARE_AXIS_06_USE_DLMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_DLMALLOC 0)
endif()

# V41.F.6.1 Batch 3 Vendor — USE = ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_HOARD AND COMDARE_HAVE_HOARD)
    set(COMDARE_AXIS_06_USE_HOARD 1)
else()
    set(COMDARE_AXIS_06_USE_HOARD 0)
endif()

if(COMDARE_AXIS_06_ENABLE_SLAB AND COMDARE_HAVE_SLAB)
    set(COMDARE_AXIS_06_USE_SLAB 1)
else()
    set(COMDARE_AXIS_06_USE_SLAB 0)
endif()

if(COMDARE_AXIS_06_ENABLE_MICHAEL_LF AND COMDARE_HAVE_MICHAEL_LF)
    set(COMDARE_AXIS_06_USE_MICHAEL_LF 1)
else()
    set(COMDARE_AXIS_06_USE_MICHAEL_LF 0)
endif()

# V41.F.6.1 Batch 4 Vendor — USE = ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_SCALLOC AND COMDARE_HAVE_SCALLOC)
    set(COMDARE_AXIS_06_USE_SCALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_SCALLOC 0)
endif()

if(COMDARE_AXIS_06_ENABLE_NUMALLOC AND COMDARE_HAVE_NUMALLOC)
    set(COMDARE_AXIS_06_USE_NUMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_NUMALLOC 0)
endif()

if(COMDARE_AXIS_06_ENABLE_RPMALLOC AND COMDARE_HAVE_RPMALLOC)
    set(COMDARE_AXIS_06_USE_RPMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_RPMALLOC 0)
endif()

# V41.F.6.1 Batch 5 Vendor — USE = ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_LRMALLOC AND COMDARE_HAVE_LRMALLOC)
    set(COMDARE_AXIS_06_USE_LRMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_LRMALLOC 0)
endif()

if(COMDARE_AXIS_06_ENABLE_CAMA AND COMDARE_HAVE_CAMA)
    set(COMDARE_AXIS_06_USE_CAMA 1)
else()
    set(COMDARE_AXIS_06_USE_CAMA 0)
endif()

if(COMDARE_AXIS_06_ENABLE_STARMALLOC AND COMDARE_HAVE_STARMALLOC)
    set(COMDARE_AXIS_06_USE_STARMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_STARMALLOC 0)
endif()

# V41.F.6.1 Batch 6 Vendor — USE = ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_TCMALLOC_WH AND COMDARE_HAVE_TCMALLOC_WH)
    set(COMDARE_AXIS_06_USE_TCMALLOC_WH 1)
else()
    set(COMDARE_AXIS_06_USE_TCMALLOC_WH 0)
endif()

if(COMDARE_AXIS_06_ENABLE_HMALLOC AND COMDARE_HAVE_HMALLOC)
    set(COMDARE_AXIS_06_USE_HMALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_HMALLOC 0)
endif()

if(COMDARE_AXIS_06_ENABLE_PIM_MALLOC AND COMDARE_HAVE_PIM_MALLOC)
    set(COMDARE_AXIS_06_USE_PIM_MALLOC 1)
else()
    set(COMDARE_AXIS_06_USE_PIM_MALLOC 0)
endif()

# V41.F.6.1 Batch 7 Vendor — USE = ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_CRYSTALLINE AND COMDARE_HAVE_CRYSTALLINE)
    set(COMDARE_AXIS_06_USE_CRYSTALLINE 1)
else()
    set(COMDARE_AXIS_06_USE_CRYSTALLINE 0)
endif()

if(COMDARE_AXIS_06_ENABLE_EXGEN AND COMDARE_HAVE_EXGEN)
    set(COMDARE_AXIS_06_USE_EXGEN 1)
else()
    set(COMDARE_AXIS_06_USE_EXGEN 0)
endif()

if(COMDARE_AXIS_06_ENABLE_BUDDY AND COMDARE_HAVE_BUDDY)
    set(COMDARE_AXIS_06_USE_BUDDY 1)
else()
    set(COMDARE_AXIS_06_USE_BUDDY 0)
endif()

# V41.F.6.1 Batch 8 Vendor — USE = ENABLE && HAVE
if(COMDARE_AXIS_06_ENABLE_PTMALLOC2 AND COMDARE_HAVE_PTMALLOC2)
    set(COMDARE_AXIS_06_USE_PTMALLOC2 1)
else()
    set(COMDARE_AXIS_06_USE_PTMALLOC2 0)
endif()

if(COMDARE_AXIS_06_ENABLE_VMEM_MAG AND COMDARE_HAVE_VMEM_MAG)
    set(COMDARE_AXIS_06_USE_VMEM_MAG 1)
else()
    set(COMDARE_AXIS_06_USE_VMEM_MAG 0)
endif()

message(STATUS "V41.F.6.1.C Allocator Axis 6 effective USE flags:")
message(STATUS "  Batch 1: USE_STD=${COMDARE_AXIS_06_USE_STD} MIMALLOC=${COMDARE_AXIS_06_USE_MIMALLOC} SNMALLOC=${COMDARE_AXIS_06_USE_SNMALLOC} PMR=${COMDARE_AXIS_06_USE_PMR}")
message(STATUS "  Batch 2: USE_JEMALLOC=${COMDARE_AXIS_06_USE_JEMALLOC} TCMALLOC=${COMDARE_AXIS_06_USE_TCMALLOC} DLMALLOC=${COMDARE_AXIS_06_USE_DLMALLOC}")
message(STATUS "  Batch 3: USE_HOARD=${COMDARE_AXIS_06_USE_HOARD} SLAB=${COMDARE_AXIS_06_USE_SLAB} MICHAEL_LF=${COMDARE_AXIS_06_USE_MICHAEL_LF}")
message(STATUS "  Batch 4: USE_SCALLOC=${COMDARE_AXIS_06_USE_SCALLOC} NUMALLOC=${COMDARE_AXIS_06_USE_NUMALLOC} RPMALLOC=${COMDARE_AXIS_06_USE_RPMALLOC}")
message(STATUS "  Batch 5: USE_LRMALLOC=${COMDARE_AXIS_06_USE_LRMALLOC} CAMA=${COMDARE_AXIS_06_USE_CAMA} STARMALLOC=${COMDARE_AXIS_06_USE_STARMALLOC}")
message(STATUS "  Batch 6: USE_TCMALLOC_WH=${COMDARE_AXIS_06_USE_TCMALLOC_WH} HMALLOC=${COMDARE_AXIS_06_USE_HMALLOC} PIM_MALLOC=${COMDARE_AXIS_06_USE_PIM_MALLOC}")
message(STATUS "  Batch 7: USE_CRYSTALLINE=${COMDARE_AXIS_06_USE_CRYSTALLINE} EXGEN=${COMDARE_AXIS_06_USE_EXGEN} BUDDY=${COMDARE_AXIS_06_USE_BUDDY}")
message(STATUS "  Batch 8: USE_PTMALLOC2=${COMDARE_AXIS_06_USE_PTMALLOC2} VMEM_MAG=${COMDARE_AXIS_06_USE_VMEM_MAG} (VOLLAUSBAU 24 Vendor)")

# V41.F.6.1 Topic queuing Pilot — USE = ENABLE direkt (kein externer Vendor heute)
# axis_Q1 buffer_strategy VOLLAUSBAU (Batch 1-5, alle 13 W2-Strategien)
foreach(_q1 NO_BUFFER FIFO LIFO BOUNDED_RING APPEND_ONLY PRIORITY_HEAP DELTA_CHAIN SKIPLIST_BUFFER TOMBSTONE_BUFFER COPY_ON_WRITE EPOCH_BUFFER BATCHED_INSERT_BUFFER LOCKFREE_SPSC LOCKFREE_MPMC)
    if(COMDARE_AXIS_Q1_ENABLE_${_q1})
        set(COMDARE_AXIS_Q1_USE_${_q1} 1)
    else()
        set(COMDARE_AXIS_Q1_USE_${_q1} 0)
    endif()
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp"
    @ONLY)

# axis_Q2 flush_policy VOLLAUSBAU (5/5 W3-Policies)
foreach(_q2 EAGER WATERMARK LAZY TIMED ADAPTIVE_LSM)
    if(COMDARE_AXIS_Q2_ENABLE_${_q2})
        set(COMDARE_AXIS_Q2_USE_${_q2} 1)
    else()
        set(COMDARE_AXIS_Q2_USE_${_q2} 0)
    endif()
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/queuing/axis_q2_queuing/axis_q2_queuing_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/queuing/axis_q2_queuing/axis_q2_queuing_flags.hpp"
    @ONLY)

message(STATUS "V41.F.6.1 Topic queuing Pilot:")
message(STATUS "  axis_Q1 VOLLAUSBAU (13/13 W2): NO_BUFFER=${COMDARE_AXIS_Q1_USE_NO_BUFFER} FIFO=${COMDARE_AXIS_Q1_USE_FIFO} LIFO=${COMDARE_AXIS_Q1_USE_LIFO} BOUNDED_RING=${COMDARE_AXIS_Q1_USE_BOUNDED_RING} APPEND_ONLY=${COMDARE_AXIS_Q1_USE_APPEND_ONLY} PRIORITY_HEAP=${COMDARE_AXIS_Q1_USE_PRIORITY_HEAP} DELTA_CHAIN=${COMDARE_AXIS_Q1_USE_DELTA_CHAIN} SKIPLIST_BUFFER=${COMDARE_AXIS_Q1_USE_SKIPLIST_BUFFER} TOMBSTONE_BUFFER=${COMDARE_AXIS_Q1_USE_TOMBSTONE_BUFFER} COPY_ON_WRITE=${COMDARE_AXIS_Q1_USE_COPY_ON_WRITE} EPOCH_BUFFER=${COMDARE_AXIS_Q1_USE_EPOCH_BUFFER} BATCHED_INSERT_BUFFER=${COMDARE_AXIS_Q1_USE_BATCHED_INSERT_BUFFER} LOCKFREE_SPSC=${COMDARE_AXIS_Q1_USE_LOCKFREE_SPSC} LOCKFREE_MPMC=${COMDARE_AXIS_Q1_USE_LOCKFREE_MPMC}")
message(STATUS "  axis_Q2 VOLLAUSBAU (5/5 W3): EAGER=${COMDARE_AXIS_Q2_USE_EAGER} WATERMARK=${COMDARE_AXIS_Q2_USE_WATERMARK} LAZY=${COMDARE_AXIS_Q2_USE_LAZY} TIMED=${COMDARE_AXIS_Q2_USE_TIMED} ADAPTIVE_LSM=${COMDARE_AXIS_Q2_USE_ADAPTIVE_LSM}")

# V41.F.6.1 Topic traversal Pilot — USE-Flag-Berechnungen + configure_file (2026-05-26)
# axis_03a search_algo
foreach(_s03a ARRAY256 VECTOR_U8U8 VECTOR_U16U16)
    if(COMDARE_AXIS_03A_ENABLE_${_s03a})
        set(COMDARE_AXIS_03A_USE_${_s03a} 1)
    else()
        set(COMDARE_AXIS_03A_USE_${_s03a} 0)
    endif()
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp"
    @ONLY)

# axis_03b cache_traversal
foreach(_s03b LINEAR_FANOUT HASH_LOOKUP)
    if(COMDARE_AXIS_03B_ENABLE_${_s03b})
        set(COMDARE_AXIS_03B_USE_${_s03b} 1)
    else()
        set(COMDARE_AXIS_03B_USE_${_s03b} 0)
    endif()
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_flags.hpp"
    @ONLY)

# axis_03m mapping
foreach(_s03m DIRECT_PLACEMENT POOL_RELATIVE)
    if(COMDARE_AXIS_03M_ENABLE_${_s03m})
        set(COMDARE_AXIS_03M_USE_${_s03m} 1)
    else()
        set(COMDARE_AXIS_03M_USE_${_s03m} 0)
    endif()
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/traversal/axis_03m_mapping/axis_03m_mapping_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/traversal/axis_03m_mapping/axis_03m_mapping_flags.hpp"
    @ONLY)

message(STATUS "V41.F.6.1 Topic traversal Pilot:")
message(STATUS "  axis_03a (3 search_algo): ARRAY256=${COMDARE_AXIS_03A_USE_ARRAY256} VECTOR_U8U8=${COMDARE_AXIS_03A_USE_VECTOR_U8U8} VECTOR_U16U16=${COMDARE_AXIS_03A_USE_VECTOR_U16U16}")
message(STATUS "  axis_03b (2 cache_traversal): LINEAR_FANOUT=${COMDARE_AXIS_03B_USE_LINEAR_FANOUT} HASH_LOOKUP=${COMDARE_AXIS_03B_USE_HASH_LOOKUP}")
message(STATUS "  axis_03m (2 mapping): DIRECT_PLACEMENT=${COMDARE_AXIS_03M_USE_DIRECT_PLACEMENT} POOL_RELATIVE=${COMDARE_AXIS_03M_USE_POOL_RELATIVE}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp"
    @ONLY)

# ─────────────────────────────────────────────────────────────────────────────
# V41.F.6.1.P2.C Zentraler Build-Switch fuer Paper-Original-Code-Validierung
# (User-Direktive: cmake flag fuer alle Funktionen, mit dem die app Originalitaet
# beweist). Bei OFF: kein Codegen, kein Mixin-Include — is_original_module()=false
# kommt generisch via AxisBase Default automatisch fuer ALLE Algorithmen.
# ─────────────────────────────────────────────────────────────────────────────
option(COMDARE_CE_ENABLE_ORIGINAL_CODE_VALIDATION
       "Enable Paper-Original-Code SHA-Validation (Habich-Compliance Pre-Build-Tool)" ON)

# ─────────────────────────────────────────────────────────────────────────────
# V41.F.6.1.P2.B/D Paper-Legacy-Code Roll-out (kompakt via Helper)
# Build-Time-Init aus ext/ in legacy_code/ + Tool-Run zur SHA-Validierung.
# Pro Paper 1 Helper-Aufruf (statt Block-Wiederholung — [[cross-axis-defaults-no-bloat]]).
# Aktiv nur wenn Validation-Option ON + Helper verfuegbar.
# Helper SKIPPED Paper automatisch wenn ext-Sentinel fehlt.
# ─────────────────────────────────────────────────────────────────────────────
if(COMDARE_CE_ENABLE_ORIGINAL_CODE_VALIDATION AND COMMAND comdare_register_paper_wrapper)
    set(_a06_legacy_root "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/allocator/axis_06_allocator/legacy_code")
    set(_a06_gen_root    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/allocator/axis_06_allocator/legacy_code")
    set(_a06_namespace_root "comdare::cache_engine::allocator::axis_06_allocator::generated")

    # A04 mimalloc (Pilot P2.B) — Leijen MSR-TR-2019-18 / ISMM 2019
    comdare_register_paper_wrapper(
        PAPER_ID          a04_mimalloc
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/A04-mimalloc"
        EXT_SENTINEL_FILE "src/alloc-aligned.c"
        LEGACY_DIR        "${_a06_legacy_root}/paper_a04_mimalloc"
        FILES             "src/alloc-aligned.c" "src/free.c"
        OUTPUT_HEADER     "${_a06_gen_root}/paper_a04_mimalloc_is_original.hpp"
        NAMESPACE         "${_a06_namespace_root}::a04_mimalloc"
        WRAPPER_NAME      mimalloc
    )

    # A05 jemalloc (P2.D Roll-out 1/3) — Evans BSDCan 2006 / Facebook Production
    comdare_register_paper_wrapper(
        PAPER_ID          a05_jemalloc
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/A05-jemalloc"
        EXT_SENTINEL_FILE "src/jemalloc.c"
        LEGACY_DIR        "${_a06_legacy_root}/paper_a05_jemalloc"
        FILES             "src/jemalloc.c"
        OUTPUT_HEADER     "${_a06_gen_root}/paper_a05_jemalloc_is_original.hpp"
        NAMESPACE         "${_a06_namespace_root}::a05_jemalloc"
        WRAPPER_NAME      jemalloc
    )

    # A07 snmalloc (P2.D Roll-out 2/3) — Lipp/Bond/Parkinson ISMM 2019
    # snmalloc nutzt C++ namespace (snmalloc::libc::*) statt extern "C" —
    # Tool findet aligned_alloc/free_aligned_sized als Identifier in libc.h.
    comdare_register_paper_wrapper(
        PAPER_ID          a07_snmalloc
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/A07-snmalloc"
        EXT_SENTINEL_FILE "src/snmalloc/global/libc.h"
        LEGACY_DIR        "${_a06_legacy_root}/paper_a07_snmalloc"
        FILES             "src/snmalloc/global/libc.h"
        OUTPUT_HEADER     "${_a06_gen_root}/paper_a07_snmalloc_is_original.hpp"
        NAMESPACE         "${_a06_namespace_root}::a07_snmalloc"
        WRAPPER_NAME      snmalloc
    )

    # A20 dlmalloc (P2.D Batch 2, 1/3) — Doug Lea 1987 Classic Bins
    comdare_register_paper_wrapper(
        PAPER_ID          a20_dlmalloc
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/A20-dlmalloc"
        EXT_SENTINEL_FILE "malloc.c"
        LEGACY_DIR        "${_a06_legacy_root}/paper_a20_dlmalloc"
        FILES             "malloc.c"
        OUTPUT_HEADER     "${_a06_gen_root}/paper_a20_dlmalloc_is_original.hpp"
        NAMESPACE         "${_a06_namespace_root}::a20_dlmalloc"
        WRAPPER_NAME      dlmalloc
    )

    # A10 rpmalloc (P2.D Batch 2, 2/3) — Jansson 2017
    comdare_register_paper_wrapper(
        PAPER_ID          a10_rpmalloc
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/A10-rpmalloc"
        EXT_SENTINEL_FILE "rpmalloc/rpmalloc.c"
        LEGACY_DIR        "${_a06_legacy_root}/paper_a10_rpmalloc"
        FILES             "rpmalloc/rpmalloc.c"
        OUTPUT_HEADER     "${_a06_gen_root}/paper_a10_rpmalloc_is_original.hpp"
        NAMESPACE         "${_a06_namespace_root}::a10_rpmalloc"
        WRAPPER_NAME      rpmalloc
    )

    # A11 lrmalloc (P2.D Batch 2, 3/3) — Leite/Rocha JPDC 2019
    comdare_register_paper_wrapper(
        PAPER_ID          a11_lrmalloc
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/A11-lrmalloc"
        EXT_SENTINEL_FILE "lrmalloc.cpp"
        LEGACY_DIR        "${_a06_legacy_root}/paper_a11_lrmalloc"
        FILES             "lrmalloc.cpp"
        OUTPUT_HEADER     "${_a06_gen_root}/paper_a11_lrmalloc_is_original.hpp"
        NAMESPACE         "${_a06_namespace_root}::a11_lrmalloc"
        WRAPPER_NAME      lrmalloc
    )

    # DEFER P2.D.t (Bazel + Custom-Shim Vendors):
    #   A06 tcmalloc, A01 hoard, A03 michael_lf, A08 scalloc — spaeterer Sprint Task #685

    # ─────────────────────────────────────────────────────────────────────────
    # V41.F.6.1.P2.D.tr s1 Traversal Roll-out — axis_03a_search_algo
    # 3 NEUE Wrappers (parallel zu existing Re-Impl Array256/VectorU8U8/VectorU16U16).
    # P02 HOT + P05 START haben Luecken (erase/clear) — Achs-Mixin defaultet auf false.
    # ─────────────────────────────────────────────────────────────────────────
    set(_a03a_legacy_root "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/traversal/axis_03a_search_algo/legacy_code")
    set(_a03a_gen_root    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/traversal/axis_03a_search_algo/legacy_code")
    set(_a03a_namespace_root "comdare::cache_engine::traversal::axis_03a_search_algo::generated")
    set(_a03a_axis_mixin     "comdare::cache_engine::traversal::axis_03a_search_algo::concepts::SearchAlgoOriginalCodeMixin")

    # P01 ART (Leis ICDE 2013) — 4/4 originall
    comdare_register_paper_wrapper(
        PAPER_ID          p01_art
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/P01-ART/unodb"
        EXT_SENTINEL_FILE "art.hpp"
        LEGACY_DIR        "${_a03a_legacy_root}/paper_p01_art"
        FILES             "art.hpp"
        OUTPUT_HEADER     "${_a03a_gen_root}/paper_p01_art_is_original.hpp"
        NAMESPACE         "${_a03a_namespace_root}::p01_art"
        WRAPPER_NAME      art
        AXIS_MIXIN_TYPE   "${_a03a_axis_mixin}"
    )

    # P02 HOT (Binna PVLDB 2018) — 2/4 originall (erase/clear Luecken)
    comdare_register_paper_wrapper(
        PAPER_ID          p02_hot
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/P02-HOT"
        EXT_SENTINEL_FILE "hot/libs/hot/rowex/include/hot/rowex/HOTRowex.hpp"
        LEGACY_DIR        "${_a03a_legacy_root}/paper_p02_hot"
        FILES             "hot/libs/hot/rowex/include/hot/rowex/HOTRowex.hpp"
        OUTPUT_HEADER     "${_a03a_gen_root}/paper_p02_hot_is_original.hpp"
        NAMESPACE         "${_a03a_namespace_root}::p02_hot"
        WRAPPER_NAME      hot
        AXIS_MIXIN_TYPE   "${_a03a_axis_mixin}"
    )

    # P05 START (Mertens ICDE 2024) — 2/4 originall (erase/clear Luecken)
    comdare_register_paper_wrapper(
        PAPER_ID          p05_start
        EXT_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/ext/P05-START"
        EXT_SENTINEL_FILE "START/sosd-competitor-adapter-START.h"
        LEGACY_DIR        "${_a03a_legacy_root}/paper_p05_start"
        FILES             "START/sosd-competitor-adapter-START.h"
        OUTPUT_HEADER     "${_a03a_gen_root}/paper_p05_start_is_original.hpp"
        NAMESPACE         "${_a03a_namespace_root}::p05_start"
        WRAPPER_NAME      start
        AXIS_MIXIN_TYPE   "${_a03a_axis_mixin}"
    )
endif()
