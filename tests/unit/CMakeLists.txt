# tests/unit/CMakeLists.txt - REV 6 Phase 6 Migration aus Diplomarbeit/code (2026-05-12)
# Unit-Tests fuer migrierte Concept-Header (Cache-Engine + PRT-ART)

# Header-only INTERFACE-Library: Cache-Engine Concepts
add_library(comdare_cache_engine_concepts INTERFACE)
add_library(comdare::cache_engine_concepts ALIAS comdare_cache_engine_concepts)
target_include_directories(comdare_cache_engine_concepts INTERFACE
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/include"
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/subsystems")
target_compile_features(comdare_cache_engine_concepts INTERFACE cxx_std_23)
comdare_set_platform_defines(comdare_cache_engine_concepts)

# Header-only INTERFACE-Library: PRT-ART Concepts
add_library(comdare_prt_art_concepts INTERFACE)
add_library(comdare::prt_art_concepts ALIAS comdare_prt_art_concepts)
target_include_directories(comdare_prt_art_concepts INTERFACE
    "${CMAKE_SOURCE_DIR}/prt_art/include")
target_compile_features(comdare_prt_art_concepts INTERFACE cxx_std_23)
target_link_libraries(comdare_prt_art_concepts INTERFACE comdare::cache_engine_concepts)
comdare_set_platform_defines(comdare_prt_art_concepts)

# Tests anlegen via Helper aus gtest_setup.cmake
comdare_add_test(test_pressure_state
    SOURCES test_pressure_state.cpp
    LIBRARIES comdare::cache_engine_concepts)

comdare_add_test(test_cache_recommendation
    SOURCES test_cache_recommendation.cpp
    LIBRARIES comdare::cache_engine_concepts)

comdare_add_test(test_cpuid_probe
    SOURCES test_cpuid_probe.cpp
    LIBRARIES comdare::cache_engine_concepts)

comdare_add_test(test_concepts_compile
    SOURCES test_concepts_compile.cpp
    LIBRARIES comdare::prt_art_concepts)

comdare_add_test(test_value_handle
    SOURCES test_value_handle.cpp
    LIBRARIES comdare::prt_art_concepts)

comdare_add_test(test_three_layer_audit
    SOURCES test_three_layer_audit.cpp
    LIBRARIES comdare::prt_art_concepts)

# INK-1 (REV 6 Phase 6, 2026-05-12): PermutationFlags + Events + Observer
comdare_add_test(test_permutation_flags
    SOURCES test_permutation_flags.cpp
    LIBRARIES comdare::cache_engine_concepts)

comdare_add_test(test_event_hierarchy
    SOURCES test_event_hierarchy.cpp
    LIBRARIES comdare::cache_engine_concepts)

comdare_add_test(test_observer_registry
    SOURCES test_observer_registry.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-2: DecisionLambdaTrees (10+ Familien) + Bundle + Registry
comdare_add_test(test_decision_trees
    SOURCES test_decision_trees.cpp
    LIBRARIES comdare::cache_engine_concepts)

comdare_add_test(test_decision_lambda_registry
    SOURCES test_decision_lambda_registry.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-3: ConcurrencyManager (10 Disziplinen + 3 Mechaniken)
comdare_add_test(test_concurrency_disciplines
    SOURCES test_concurrency_disciplines.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-4: TelemetryStrategy Achse 11 Kuehn (5 Strategien + ProbabilityHints)
comdare_add_test(test_telemetry_strategies
    SOURCES test_telemetry_strategies.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-5: Plattform-Auto-Discovery (REV 3 K3.2, REV 5 K07/K08)
comdare_add_test(test_platform_concepts
    SOURCES test_platform_concepts.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-6: CacheHierarchyManager + CacheEngineMode + Scheduler + Heuristik
comdare_add_test(test_cache_hierarchy_and_mode
    SOURCES test_cache_hierarchy_and_mode.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-7: HybridCompositionCommand (Command-Pattern, REV 3 K3.4)
comdare_add_test(test_strategy_command
    SOURCES test_strategy_command.cpp
    LIBRARIES comdare::cache_engine_concepts)

# INK-8: InMemoryMeasurementBuffer (F5) + Measure (F1) + F-EXTRA-7 + F11
comdare_add_test(test_measurement_buffer
    SOURCES test_measurement_buffer.cpp
    LIBRARIES comdare::cache_engine_concepts)

# Sub-Engine-Slots C01-C12 (Termin 7 + RBMM-Konvention)
comdare_add_test(test_sub_engines
    SOURCES test_sub_engines.cpp
    LIBRARIES comdare::cache_engine_concepts)

# 6 Pflicht-Seitentypen (REV 5 K05) — ISearchPageStructure + ISearchPageStructureInterpreter
comdare_add_test(test_six_page_structures
    SOURCES test_six_page_structures.cpp
    LIBRARIES comdare::prt_art_concepts)

# Phase 6.2.E (REV 7, 2026-05-13): Allokator-Bibliothek Concept-Wurzeln + 6 Family-Adapter
comdare_add_test(test_allocator_concepts
    SOURCES test_allocator_concepts.cpp
    LIBRARIES comdare::cache_engine_concepts)

# Phase 6.3 (REV 7, 2026-05-13): ABI-stabiles C++23-Modul-Interface
# type_collection_traits + search_algorithm_type_collection + fingerprint +
# execution_engine + search_engine + resolve_baustein + module_abi_v1
comdare_add_test(test_abi_interface
    SOURCES test_abi_interface.cpp
    LIBRARIES comdare::cache_engine_concepts)

# Phase 6.5 (REV 7, 2026-05-13): TestDataSetAccumulationEngine
comdare_add_test(test_test_data_accumulation
    SOURCES test_test_data_accumulation.cpp
    LIBRARIES comdare::test_data_accumulation)

# Phase 6.6 (REV 7, 2026-05-13): Mikrobenchmark-Suite
# No-deprecate Wrapper + 2 Custom-Allokationen + Sparse-Binary-Logging + Conversions
comdare_add_test(test_benchmark_suite
    SOURCES test_benchmark_suite.cpp
    LIBRARIES comdare::benchmark_suite)

# Phase A (REV 7 Pflicht §3.2): Vollstaendige Container-Matrix x 10 Adapter +
# Threading-Tests (Single-W/Multi-R + Multi-Threaded-Stress)
comdare_add_test(test_allocator_full_matrix
    SOURCES test_allocator_full_matrix.cpp
    LIBRARIES comdare::cache_engine_concepts)

# Phase 7 (REV 7 §9, 2026-05-13): Workload-Generator + ResultAggregator + Demo
comdare_add_test(test_workload_and_experiment
    SOURCES test_workload_and_experiment.cpp
    LIBRARIES comdare::cache_engine_concepts comdare::workload_generator comdare::experiment)

# Phase C (2026-05-13): YCSB-CLI Tool
comdare_add_test(test_ycsb_cli
    SOURCES test_ycsb_cli.cpp
    LIBRARIES comdare::ycsb_cli_lib comdare::workload_generator)

# Phase 7.2 (2026-05-13): CacheEngineBuilder Codegen + Aggregator-CMakeLists.txt
comdare_add_test(test_builder_codegen
    SOURCES test_builder_codegen.cpp
    LIBRARIES comdare_builder_codegen)

# REV 7.6 V10.4 (2026-05-14): Codegen-Profil-Roundtrip-Tests fuer V9.3-API
comdare_add_test(test_codegen_from_profile
    SOURCES test_codegen_from_profile.cpp
    LIBRARIES comdare_builder_codegen)

# REV 7.6 V14.2 (2026-05-14): ExperimentDriver V13 API-Tests
comdare_add_test(test_experiment_driver_v13
    SOURCES test_experiment_driver_v13.cpp
    LIBRARIES comdare_builder_experiment_driver)

# Phase 8 (2026-05-13): LaTeX-Anhang-Tool
comdare_add_test(test_latex_anhang
    SOURCES test_latex_anhang.cpp
    LIBRARIES comdare::latex_anhang_lib)

# Aufgabe #104 (2026-05-13): eigene RCU-Implementation
comdare_add_test(test_rcu
    SOURCES test_rcu.cpp
    LIBRARIES comdare::rcu)

# Aufgabe #105 (2026-05-13): eigene SDSL-Lite-Portierung (BitVector + LOUDS)
comdare_add_test(test_succinct
    SOURCES test_succinct.cpp
    LIBRARIES comdare::succinct)

# Aufgabe #106 (2026-05-13): HBM-Hybrid-Cache-Hierarchie + Abstract Factory
comdare_add_test(test_hbm_hierarchy
    SOURCES test_hbm_hierarchy.cpp
    LIBRARIES comdare::cache_engine_concepts)

# Phase 7.2.D (2026-05-13): ModuleLoader fuer Permutations-DLLs
# Mock-DLL als SHARED-Library, die das ABI exportiert
add_library(comdare_perm_mock_test SHARED mock_permutation_module.cpp)
target_compile_features(comdare_perm_mock_test PRIVATE cxx_std_23)
target_include_directories(comdare_perm_mock_test PRIVATE
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/include")
set_target_properties(comdare_perm_mock_test PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

comdare_add_test(test_module_loader
    SOURCES test_module_loader.cpp
    LIBRARIES comdare_module_loader)
target_compile_definitions(test_module_loader PRIVATE
    "COMDARE_MOCK_DLL_PATH=\"$<TARGET_FILE:comdare_perm_mock_test>\"")
add_dependencies(test_module_loader comdare_perm_mock_test)

# V31.F (2026-05-14): Smoke-Tests fuer alle 22 V31-Adapter
comdare_add_test(test_v31_adapters
    SOURCES test_v31_adapters.cpp
    LIBRARIES
        comdare::adapter::p01_art
        comdare::adapter::p02_hot
        comdare::adapter::p03_masstree
        comdare::adapter::p04_cocotrie
        comdare::adapter::p05_start
        comdare::adapter::p06_b2tree
        comdare::adapter::p07_wormhole
        comdare::adapter::p10_surf
        comdare::adapter::p20_btreesareback
        comdare::adapter::p25_mahling
        comdare::adapter::p29_rcu
        comdare::adapter::p30_hazardpointers
        comdare::adapter::a01_hoard
        comdare::adapter::a03_michael_lockfree
        comdare::adapter::a04_mimalloc
        comdare::adapter::a05_jemalloc
        comdare::adapter::a06_tcmalloc
        comdare::adapter::a07_snmalloc
        comdare::adapter::a08_scalloc
        comdare::adapter::a10_rpmalloc
        comdare::adapter::a11_lrmalloc
        comdare::adapter::a20_dlmalloc)
target_include_directories(test_v31_adapters PRIVATE "${CMAKE_SOURCE_DIR}")

# V41.A5 (2026-05-24): Echte Index-Implementierungen
comdare_add_test(test_v41_indexes
    SOURCES test_v41_indexes.cpp)
target_include_directories(test_v41_indexes PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# V41.F.6.1.A (2026-05-25): Topic+Achsen-Concept Pilot (Allocator Axis 6)
# V41.F.6.1.C Stufe 1: + Boost::mp11 fuer axis_06_allocator_registry.hpp
# V41.F.6.1.C Stufe 1: + generated/ Pfad fuer axis_06_allocator_flags.hpp
comdare_add_test(test_v41_topic_allocator_axis_06
    SOURCES test_v41_topic_allocator_axis_06.cpp)
target_include_directories(test_v41_topic_allocator_axis_06 PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"  # V41.F.6.1 Stufe 3: measurement/measurable_concept.hpp
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_topic_allocator_axis_06 PRIVATE Boost::mp11)

# V41.F.6.1 Topic queuing Pilot (2026-05-26)
comdare_add_test(test_v41_topic_queuing
    SOURCES test_v41_topic_queuing.cpp)
target_include_directories(test_v41_topic_queuing PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_topic_queuing PRIVATE Boost::mp11)

# V41.F.6.1 Topic traversal Pilot (2026-05-26)
comdare_add_test(test_v41_topic_traversal
    SOURCES test_v41_topic_traversal.cpp)
target_include_directories(test_v41_topic_traversal PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_topic_traversal PRIVATE Boost::mp11)

# V41.F.6.1.P1 Phase B Paper-Legacy-Code Pattern (ctsha + Sub-Concept + Macro)
comdare_add_test(test_v41_paper_legacy_code
    SOURCES test_v41_paper_legacy_code.cpp)
target_include_directories(test_v41_paper_legacy_code PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_paper_legacy_code PRIVATE Boost::mp11)

# V41.F.6.1.R2 Composition-Templates (Architektur-Refactoring nach Organ-Metapher)
comdare_add_test(test_v41_compositions
    SOURCES test_v41_compositions.cpp)
target_include_directories(test_v41_compositions PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_compositions PRIVATE Boost::mp11)
