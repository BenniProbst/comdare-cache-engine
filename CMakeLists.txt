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

# ─────────────────────────────────────────────────────────────────────────────
# Multi-OS-Infrastruktur (REV 6 Migration aus Diplomarbeit/code, 2026-05-12)
# ─────────────────────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(platform_detection)
include(compiler_flags)
include(isa_features)
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
  if(EXISTS "${CMAKE_BINARY_DIR}/generated/permutations.cmake")
    include("${CMAKE_BINARY_DIR}/generated/permutations.cmake")
  endif()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Subdirectories (Domaenen 1-6)
# ─────────────────────────────────────────────────────────────────────────────
add_subdirectory(measurement)        # Domaene 3 (zuerst, wird ueberall genutzt)
add_subdirectory(hardware_isa)       # Domaene 4
add_subdirectory(search_engine)      # Domaene 1
add_subdirectory(cache_engine)       # Domaene 2 (nutzt 1, 3, 4)
add_subdirectory(engine_choice)      # Domaene 5
add_subdirectory(prt_art)            # PRT-ART Test-Algorithmus (eigen)
add_subdirectory(adapters)           # Habich-Direktive Adapter
add_subdirectory(ext)                # Originalcode (mit Original-Compilern)
add_subdirectory(test_data_accumulation)  # Phase 6.5 (REV 7 §7)
add_subdirectory(benchmark_suite)         # Phase 6.6 (REV 7 §8)

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
message(STATUS "──────────────────────────────────────────────────────────")
