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
