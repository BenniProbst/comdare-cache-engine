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

# V41.F.6.1.F Topics-Fundament-Sprint (12 Topics Skelett-Stufe-A)
comdare_add_test(test_v41_topics_fundament
    SOURCES test_v41_topics_fundament.cpp)
target_include_directories(test_v41_topics_fundament PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_topics_fundament PRIVATE Boost::mp11)

# V41.F.6.1.R3 SearchAlgorithmAnatomy Skelett (Saeugetier-Anatomie)
comdare_add_test(test_v41_anatomy
    SOURCES test_v41_anatomy.cpp)
target_include_directories(test_v41_anatomy PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy PRIVATE Boost::mp11)

# V41.F.6.1.R4 AnatomyPermutationDriver (PermutationEngine + Anatomie-Brueckenkopf)
comdare_add_test(test_v41_anatomy_r4_driver
    SOURCES test_v41_anatomy_r4_driver.cpp)
target_include_directories(test_v41_anatomy_r4_driver PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_r4_driver PRIVATE Boost::mp11)

# V41.F.6.1.R5.A ObserverAggregate (ABI-stabiler 17-Achsen-Snapshot)
comdare_add_test(test_v41_anatomy_observer
    SOURCES test_v41_anatomy_observer.cpp)
target_include_directories(test_v41_anatomy_observer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_observer PRIVATE Boost::mp11)

# V41.F.6.1.R5.B Builder-Anatomy-Commands (insert/lookup/erase/clear/observe)
comdare_add_test(test_v41_builder_anatomy_commands
    SOURCES test_v41_builder_anatomy_commands.cpp)
target_include_directories(test_v41_builder_anatomy_commands PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_builder_anatomy_commands PRIVATE Boost::mp11)

# V41.F.6.1.R5.C Pruefling-Merge (3 Kompositionale Joins Stufe 1/2/3)
comdare_add_test(test_v41_anatomy_pruefling_merge
    SOURCES test_v41_anatomy_pruefling_merge.cpp)
target_include_directories(test_v41_anatomy_pruefling_merge PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_pruefling_merge PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.A AnatomyBase Wurzel + Gattungs-Marker (Mammal/Bird/Reptile/Invertebrate/Plant)
comdare_add_test(test_v41_anatomy_base
    SOURCES test_v41_anatomy_base.cpp)
target_include_directories(test_v41_anatomy_base PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_base PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.A2 ExecutionEngine als Wurzel ueber AnatomyBase (Lebewesen vs Viren)
comdare_add_test(test_v41_execution_engine
    SOURCES test_v41_execution_engine.cpp)
target_include_directories(test_v41_execution_engine PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_execution_engine PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.B SearchAlgorithmPermutationEngine (genus-aware Specialization)
comdare_add_test(test_v41_search_algorithm_permutation_engine
    SOURCES test_v41_search_algorithm_permutation_engine.cpp)
target_include_directories(test_v41_search_algorithm_permutation_engine PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_search_algorithm_permutation_engine PRIVATE Boost::mp11)

# V41.F.6.1.R5.D Anatomy Module ABI v1 (extern "C" Factory + Macro-Expansion)
comdare_add_test(test_v41_anatomy_module_abi
    SOURCES test_v41_anatomy_module_abi.cpp)
target_include_directories(test_v41_anatomy_module_abi PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_module_abi PRIVATE Boost::mp11)

# V41.F.6.1.R5.D.2 anatomy_codegen Pilot (HotComposition Static-Lib + Test)
# Generiert via configure_file aus
#   libs/cache_engine/builder/codegen/templates/anatomy_permutation_module.cpp.in
# eine STATIC-Lib mit den 4 Pflicht-extern-"C"-Symbolen fuer HotComposition.
include(anatomy_codegen)
comdare_codegen_anatomy_module(
    TARGET_NAME         anatomy_codegen_pilot_hot
    COMPOSITION_TYPE    "::comdare::cache_engine::compositions::HotComposition"
    COMPOSITION_HEADER  "compositions/hot_reference.hpp"
    LIBRARY_TYPE        STATIC
    OUTPUT_DIR          "${CMAKE_BINARY_DIR}/generated/anatomy_modules"
    FINGERPRINT         "hot_pilot")

comdare_add_test(test_v41_anatomy_codegen
    SOURCES test_v41_anatomy_codegen.cpp)
target_include_directories(test_v41_anatomy_codegen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_codegen PRIVATE
    Boost::mp11
    anatomy_codegen_pilot_hot)

# V41.F.6.1.R5.E AnatomyModuleLoader (dlopen/LoadLibrary SHARED-Pilot via WormholeComposition)
# Pilot wird als SHARED-Lib generiert + Test laedt via dlopen.
comdare_codegen_anatomy_module(
    TARGET_NAME         anatomy_codegen_pilot_wormhole_shared
    COMPOSITION_TYPE    "::comdare::cache_engine::compositions::WormholeComposition"
    COMPOSITION_HEADER  "compositions/wormhole_reference.hpp"
    LIBRARY_TYPE        SHARED
    OUTPUT_DIR          "${CMAKE_BINARY_DIR}/generated/anatomy_modules"
    FINGERPRINT         "wormhole_pilot")

comdare_add_test(test_v41_anatomy_module_loader
    SOURCES test_v41_anatomy_module_loader.cpp)
target_include_directories(test_v41_anatomy_module_loader PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_module_loader PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
# Pfad zur Pilot-DLL via Compile-Define + Build-Dependency auf SHARED-Pilot
target_compile_definitions(test_v41_anatomy_module_loader PRIVATE
    "COMDARE_R5E_PILOT_DLL=\"$<TARGET_FILE:anatomy_codegen_pilot_wormhole_shared>\"")
add_dependencies(test_v41_anatomy_module_loader anatomy_codegen_pilot_wormhole_shared)

# V41.F.6.1.R5.D.3 Multi-Permutation-Codegen + load_all (3 Compositions als SHARED-DLLs)
set(_r5d3_pilot_dir "${CMAKE_BINARY_DIR}/generated/anatomy_modules_multi")
comdare_codegen_anatomy_module_list(
    PILOT_PREFIX  "anatomy_multi_pilot"
    OUTPUT_DIR    "${_r5d3_pilot_dir}"
    LIBRARY_TYPE  SHARED
    COMPOSITIONS
        "::comdare::cache_engine::compositions::ArtComposition|compositions/art_reference.hpp"
        "::comdare::cache_engine::compositions::StartComposition|compositions/start_reference.hpp"
        "::comdare::cache_engine::compositions::SurfComposition|compositions/surf_reference.hpp"
    TARGETS_OUT _r5d3_pilot_targets)

comdare_add_test(test_v41_anatomy_multi_codegen
    SOURCES test_v41_anatomy_multi_codegen.cpp)
target_include_directories(test_v41_anatomy_multi_codegen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_multi_codegen PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
target_compile_definitions(test_v41_anatomy_multi_codegen PRIVATE
    "COMDARE_R5D3_PILOT_DIR=\"${_r5d3_pilot_dir}\"")
add_dependencies(test_v41_anatomy_multi_codegen ${_r5d3_pilot_targets})

# V41.F.6.1.R5.F anatomy_codegen_tool Library-Tests
comdare_add_test(test_v41_anatomy_codegen_tool
    SOURCES test_v41_anatomy_codegen_tool.cpp)
target_include_directories(test_v41_anatomy_codegen_tool PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_codegen_tool PRIVATE
    comdare::anatomy_codegen_tool)

# V41.F.6.1.R5.G Composition-Location-Traits + descriptor_from_composition Tests
comdare_add_test(test_v41_anatomy_codegen_tool_traits
    SOURCES test_v41_anatomy_codegen_tool_traits.cpp)
target_include_directories(test_v41_anatomy_codegen_tool_traits PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_codegen_tool_traits PRIVATE
    Boost::mp11
    comdare::anatomy_codegen_tool)

# V41.F.6.1.R5.J mp_list-driven Tool-Tabelle Tests
comdare_add_test(test_v41_anatomy_codegen_tool_mp_list
    SOURCES test_v41_anatomy_codegen_tool_mp_list.cpp)
target_include_directories(test_v41_anatomy_codegen_tool_mp_list PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_codegen_tool_mp_list PRIVATE
    Boost::mp11
    comdare::anatomy_codegen_tool)

# V41.F.6.1.R6.A WorkloadConfig + deterministischer WorkloadGenerator
comdare_add_test(test_v41_workload_generator
    SOURCES test_v41_workload_generator.cpp)
target_include_directories(test_v41_workload_generator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_v41_workload_generator PRIVATE
    comdare::workload_driver)

# V41.F.6.1.R7.1 axis_12 General-Hardware (Foundation-Sprint Wrapper-Vollausbau)
# R7.1.a.2 Goldstandard-Nachruestung: braucht Boost::mp11 (Registry+TopicConfigSet)
# und configure_file Output (axis_12_general_hardware_flags.hpp im generated/)
comdare_add_test(test_v41_axis_12_general_hardware
    SOURCES test_v41_axis_12_general_hardware.cpp)
target_include_directories(test_v41_axis_12_general_hardware PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware")
target_link_libraries(test_v41_axis_12_general_hardware PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.1.b axis_05 Memory-Layout (4 Wrappers, Goldstandard-konform)
comdare_add_test(test_v41_axis_05_memory_layout
    SOURCES test_v41_axis_05_memory_layout.cpp)
target_include_directories(test_v41_axis_05_memory_layout PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/memory_layout/axis_05_memory_layout")
target_link_libraries(test_v41_axis_05_memory_layout PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.1.c+d axis_02 + axis_04 nodes (Goldstandard-Vollausbau in einem Test-File)
comdare_add_test(test_v41_axis_02_axis_04_nodes
    SOURCES test_v41_axis_02_axis_04_nodes.cpp)
target_include_directories(test_v41_axis_02_axis_04_nodes PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/nodes/axis_02_path_compression"
    "${CMAKE_BINARY_DIR}/generated/topics/nodes/axis_04_node_type")
target_link_libraries(test_v41_axis_02_axis_04_nodes PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.a axis_07 Prefetch (4 Wrappers, Optional-Topics Start)
comdare_add_test(test_v41_axis_07_prefetch
    SOURCES test_v41_axis_07_prefetch.cpp)
target_include_directories(test_v41_axis_07_prefetch PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/prefetch/axis_07_prefetch")
target_link_libraries(test_v41_axis_07_prefetch PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.b axis_11 Telemetry (4 Wrappers, Optional-Topics)
comdare_add_test(test_v41_axis_11_telemetry
    SOURCES test_v41_axis_11_telemetry.cpp)
target_include_directories(test_v41_axis_11_telemetry PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/telemetry/axis_11_telemetry")
target_link_libraries(test_v41_axis_11_telemetry PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.c axis_10 Serialization (4 Wrappers, Optional-Topics)
comdare_add_test(test_v41_axis_10_serialization
    SOURCES test_v41_axis_10_serialization.cpp)
target_include_directories(test_v41_axis_10_serialization PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/serialization/axis_10_serialization")
target_link_libraries(test_v41_axis_10_serialization PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.d axis_14 ValueHandle (4 Wrappers, Optional-Topics)
comdare_add_test(test_v41_axis_14_value_handle
    SOURCES test_v41_axis_14_value_handle.cpp)
target_include_directories(test_v41_axis_14_value_handle PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/value_handle/axis_14_value_handle")
target_link_libraries(test_v41_axis_14_value_handle PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.e axis_filter (4 Wrappers + TYPED_TEST_SUITE Pattern aus axis_06_allocator)
comdare_add_test(test_v41_axis_filter
    SOURCES test_v41_axis_filter.cpp)
target_include_directories(test_v41_axis_filter PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/filter/axis_filter")
target_link_libraries(test_v41_axis_filter PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.f axis_io (4 Wrappers + TYPED_TEST_SUITE Pattern)
comdare_add_test(test_v41_axis_io
    SOURCES test_v41_axis_io.cpp)
target_include_directories(test_v41_axis_io PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/io/axis_io")
target_link_libraries(test_v41_axis_io PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.g axis_migration (4 Wrappers + TYPED_TEST_SUITE Pattern)
comdare_add_test(test_v41_axis_migration
    SOURCES test_v41_axis_migration.cpp)
target_include_directories(test_v41_axis_migration PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/migration/axis_migration")
target_link_libraries(test_v41_axis_migration PRIVATE
    Boost::mp11)

# V41.F.6.1.R5.I Tool zur Configure-Time aufrufen + Snippet inline include
# 2-Pass-Build: 1. cmake-Pass (Tool noch nicht gebaut, Warning) -> Tool bauen ->
# 2. cmake-Pass (Tool da, Snippet wird generiert + included)
include(anatomy_codegen_runner)
set(_r5i_snippet "${CMAKE_BINARY_DIR}/generated/r5i_perm_list.cmake")
set(_r5i_pilot_dir "${CMAKE_BINARY_DIR}/generated/r5i_anatomy_modules")
comdare_run_anatomy_codegen_tool(
    OUTPUT       "${_r5i_snippet}"
    NAMES        "art,hot,wormhole"
    LIBRARY_TYPE SHARED
    STATUS_OUT   _r5i_status)

if(_r5i_status STREQUAL "FOUND" AND EXISTS "${_r5i_snippet}")
    include("${_r5i_snippet}")
    # Pilot-DLLs aus Tool-erzeugter Composition-Liste bauen
    comdare_codegen_anatomy_module_list(
        PILOT_PREFIX  "r5i_configure_pilot"
        OUTPUT_DIR    "${_r5i_pilot_dir}"
        LIBRARY_TYPE  ${COMDARE_PERMUTATION_LIBRARY_TYPE}
        COMPOSITIONS  ${COMDARE_PERMUTATION_COMPOSITIONS}
        TARGETS_OUT   _r5i_pilot_targets)

    comdare_add_test(test_v41_anatomy_r5i_configure_codegen
        SOURCES test_v41_anatomy_r5i_configure_codegen.cpp)
    target_include_directories(test_v41_anatomy_r5i_configure_codegen PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${CMAKE_BINARY_DIR}/generated")
    target_link_libraries(test_v41_anatomy_r5i_configure_codegen PRIVATE
        Boost::mp11
        comdare::anatomy_module_loader)
    target_compile_definitions(test_v41_anatomy_r5i_configure_codegen PRIVATE
        "COMDARE_R5I_PILOT_DIR=\"${_r5i_pilot_dir}\"")
    add_dependencies(test_v41_anatomy_r5i_configure_codegen ${_r5i_pilot_targets})

    message(STATUS "R5.I: configure-time codegen ACTIVE (test target enabled)")
else()
    message(STATUS
        "R5.I: configure-time codegen SKIPPED (status=${_r5i_status}). "
        "Build the tool first: cmake --build . --target comdare_anatomy_codegen_cli, "
        "then re-configure to enable the R5.I pilot test.")
endif()
