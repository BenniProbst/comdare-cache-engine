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
# ... weitere Vendor-Flags fuer Batch 7-8 (A17-A19,A21,A23) folgen

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

message(STATUS "V41.F.6.1.C Allocator Axis 6 effective USE flags:")
message(STATUS "  Batch 1: USE_STD=${COMDARE_AXIS_06_USE_STD} MIMALLOC=${COMDARE_AXIS_06_USE_MIMALLOC} SNMALLOC=${COMDARE_AXIS_06_USE_SNMALLOC} PMR=${COMDARE_AXIS_06_USE_PMR}")
message(STATUS "  Batch 2: USE_JEMALLOC=${COMDARE_AXIS_06_USE_JEMALLOC} TCMALLOC=${COMDARE_AXIS_06_USE_TCMALLOC} DLMALLOC=${COMDARE_AXIS_06_USE_DLMALLOC}")
message(STATUS "  Batch 3: USE_HOARD=${COMDARE_AXIS_06_USE_HOARD} SLAB=${COMDARE_AXIS_06_USE_SLAB} MICHAEL_LF=${COMDARE_AXIS_06_USE_MICHAEL_LF}")
message(STATUS "  Batch 4: USE_SCALLOC=${COMDARE_AXIS_06_USE_SCALLOC} NUMALLOC=${COMDARE_AXIS_06_USE_NUMALLOC} RPMALLOC=${COMDARE_AXIS_06_USE_RPMALLOC}")
message(STATUS "  Batch 5: USE_LRMALLOC=${COMDARE_AXIS_06_USE_LRMALLOC} CAMA=${COMDARE_AXIS_06_USE_CAMA} STARMALLOC=${COMDARE_AXIS_06_USE_STARMALLOC}")
message(STATUS "  Batch 6: USE_TCMALLOC_WH=${COMDARE_AXIS_06_USE_TCMALLOC_WH} HMALLOC=${COMDARE_AXIS_06_USE_HMALLOC} PIM_MALLOC=${COMDARE_AXIS_06_USE_PIM_MALLOC}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp"
    @ONLY)
