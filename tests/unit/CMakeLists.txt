# tests/unit/CMakeLists.txt - REV 6 Phase 6 Migration aus Diplomarbeit/code (2026-05-12)
# Unit-Tests fuer migrierte Concept-Header (Cache-Engine + PRT-ART)

# V41.F.6.1.R7.3-buildfix — alle generierten axis_*_flags.hpp include-paths.
# ROBUST gegen Configure-Ordering: Pfade werden aus dem SOURCE-Tree abgeleitet (existiert
# IMMER), NICHT per GLOB des Binary-Trees. Grund: add_subdirectory(tests) (Root-CMakeLists
# Z.359) laeuft VOR den configure_file()-Aufrufen (Z.575+), die die generated/topics/<...>-
# Dirs erst erzeugen. Ein Binary-Tree-GLOB war daher beim 1. Configure leer -> "2x-configure"-
# Bug (Composition-Tests fanden axis_NN_*_flags.hpp nicht). Die Binary-Dirs existieren zur
# BUILD-Zeit (configure_file laeuft im selben Configure) — daher genuegt die Pfad-Ableitung.
file(GLOB _comdare_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(COMDARE_ALL_AXIS_GENERATED_DIRS "")
foreach(_axis_src_dir ${_comdare_axis_src_dirs})
    if(IS_DIRECTORY "${_axis_src_dir}")
        file(RELATIVE_PATH _axis_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_axis_src_dir}")
        list(APPEND COMDARE_ALL_AXIS_GENERATED_DIRS
            "${CMAKE_BINARY_DIR}/generated/topics/${_axis_rel}")
    endif()
endforeach()
# V41.F.2: physisch nach axes/<axis>/ migrierte Achsen (Glob topics/*/axis_* findet sie nicht) — generisch
# je migriertem axes/<axis>-Verzeichnis das generated-Dir ergaenzen, sonst C1083 in umbrella/compositions.
file(GLOB _comdare_axes_dirs LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_axes_dir ${_comdare_axes_dirs})
    if(IS_DIRECTORY "${_axes_dir}")
        get_filename_component(_axes_name "${_axes_dir}" NAME)
        list(APPEND COMDARE_ALL_AXIS_GENERATED_DIRS "${CMAKE_BINARY_DIR}/generated/axes/${_axes_name}")
    endif()
endforeach()

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

# V41.F.6.1 E1 (2026-05-29) — Legacy-prt_art-Concept-Tests gaten: sie inkludieren
# prt_art/concepts|page_structures-Header, die in der F.6-Migration nach cache-engine-Achsen
# verschoben wurden und im STANDALONE-Build (ohne befuelltes prt-art-Submodul) NICHT vorhanden sind
# → wuerden mit C1083 als _NOT_BUILT scheitern. Root-cause-korrekt: nur registrieren, wenn die
# prt-art-Legacy-Header tatsaechlich da sind (mit COMDARE_CE_PRUEFLINGE/prt-art-Kontext bauen sie).
set(COMDARE_PRT_ART_LEGACY_AVAILABLE FALSE)
if(EXISTS "${CMAKE_SOURCE_DIR}/prt_art/include/prt_art/prt_art.hpp")
    set(COMDARE_PRT_ART_LEGACY_AVAILABLE TRUE)
endif()

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

# V41.F.6.1 E1: prt-art-Legacy-Concept-Tests nur mit verfuegbaren prt-art-Headern (sonst _NOT_BUILT).
if(COMDARE_PRT_ART_LEGACY_AVAILABLE)
    comdare_add_test(test_concepts_compile
        SOURCES test_concepts_compile.cpp
        LIBRARIES comdare::prt_art_concepts)

    comdare_add_test(test_value_handle
        SOURCES test_value_handle.cpp
        LIBRARIES comdare::prt_art_concepts)

    comdare_add_test(test_three_layer_audit
        SOURCES test_three_layer_audit.cpp
        LIBRARIES comdare::prt_art_concepts)
else()
    message(STATUS "tests/unit: prt-art-Legacy-Concept-Tests (concepts_compile/value_handle/three_layer_audit) "
        "uebersprungen — prt-art-Header nicht im Standalone-Build (mit COMDARE_CE_PRUEFLINGE verfuegbar).")
endif()

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

# 6 Pflicht-Seitentypen (REV 5 K05) — Legacy-prt_art-Test (page_structures); die migrierten 6
# Seitentypen sind aktuell durch test_v41_axis_01_page_type abgedeckt. Nur mit prt-art-Headern bauen.
if(COMDARE_PRT_ART_LEGACY_AVAILABLE)
    comdare_add_test(test_six_page_structures
        SOURCES test_six_page_structures.cpp
        LIBRARIES comdare::prt_art_concepts)
endif()

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
    "${CMAKE_BINARY_DIR}/generated"
    # V41 Saeule-1 (Doku 24 §6): der traversal-Test komponiert das Traversal-Organ mit dem
    # nodes-axis_04 Storage-Organ (NodeTypeSlotStore<Node4NodeType>) -> braucht die generierten
    # Flags aller Achsen (wie compositions/anatomy-Tests). Folge-Increments (layout/allocator) inklusive.
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_topic_traversal PRIVATE Boost::mp11)

# V41.F.6.1.P1 Phase B Paper-Legacy-Code Pattern (ctsha + Sub-Concept + Macro)
comdare_add_test(test_v41_paper_legacy_code
    SOURCES test_v41_paper_legacy_code.cpp)
target_include_directories(test_v41_paper_legacy_code PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_paper_legacy_code PRIVATE Boost::mp11)

# V41.F.6.1.R2 Composition-Templates (Architektur-Refactoring nach Organ-Metapher)
comdare_add_test(test_v41_compositions
    SOURCES test_v41_compositions.cpp)
target_include_directories(test_v41_compositions PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_compositions PRIVATE Boost::mp11)

# V41.F.6.1.F Topics-Fundament-Sprint (12 Topics Skelett-Stufe-A)
comdare_add_test(test_v41_topics_fundament
    SOURCES test_v41_topics_fundament.cpp)
target_include_directories(test_v41_topics_fundament PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_topics_fundament PRIVATE Boost::mp11)

# V41.F.6.1.R3 SearchAlgorithmAnatomy Skelett (Saeugetier-Anatomie)
comdare_add_test(test_v41_anatomy
    SOURCES test_v41_anatomy.cpp)
target_include_directories(test_v41_anatomy PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy PRIVATE Boost::mp11)

# V41.F.6.1.R4 AnatomyPermutationDriver (PermutationEngine + Anatomie-Brueckenkopf)
comdare_add_test(test_v41_anatomy_r4_driver
    SOURCES test_v41_anatomy_r4_driver.cpp)
target_include_directories(test_v41_anatomy_r4_driver PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_r4_driver PRIVATE Boost::mp11)

# V41.F.6.1.R5.A ObserverAggregate (ABI-stabiler 17-Achsen-Snapshot)
comdare_add_test(test_v41_anatomy_observer
    SOURCES test_v41_anatomy_observer.cpp)
target_include_directories(test_v41_anatomy_observer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_observer PRIVATE Boost::mp11)

# V41.F.6.1.R5.B Builder-Anatomy-Commands (insert/lookup/erase/clear/observe)
comdare_add_test(test_v41_builder_anatomy_commands
    SOURCES test_v41_builder_anatomy_commands.cpp)
target_include_directories(test_v41_builder_anatomy_commands PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_builder_anatomy_commands PRIVATE Boost::mp11)

# V41 Roadmap-3 (Doku 24 §2.1/§2.2) Saeule-2 Mess-Pfad: TierObserveTrace Fuellstands-Treiber
comdare_add_test(test_v41_tier_observe_trace
    SOURCES test_v41_tier_observe_trace.cpp)
target_include_directories(test_v41_tier_observe_trace PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_tier_observe_trace PRIVATE Boost::mp11)

# V41 Roadmap-4 (Doku 24 §2.3) Saeule 3: Achsen-Vergleich gegen vereinheitlichtes std::map-Interface
comdare_add_test(test_v41_axis_03a_cross_variant_equivalence
    SOURCES test_v41_axis_03a_cross_variant_equivalence.cpp)
target_include_directories(test_v41_axis_03a_cross_variant_equivalence PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_03a_cross_variant_equivalence PRIVATE Boost::mp11)

# V41 #40 (Doku 24 §6.2) Tier->Organ-Rekonstruktions-Beleg (Tier ≡ Organ-Komposition ≡ std::map)
comdare_add_test(test_v41_axis_03a_tier_organ_equivalence
    SOURCES test_v41_axis_03a_tier_organ_equivalence.cpp)
target_include_directories(test_v41_axis_03a_tier_organ_equivalence PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_03a_tier_organ_equivalence PRIVATE Boost::mp11)

# V41.F.6.1.R5.C Pruefling-Merge (3 Kompositionale Joins Stufe 1/2/3)
comdare_add_test(test_v41_anatomy_pruefling_merge
    SOURCES test_v41_anatomy_pruefling_merge.cpp)
target_include_directories(test_v41_anatomy_pruefling_merge PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_pruefling_merge PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.A AnatomyBase Wurzel + Gattungs-Marker (Mammal/Bird/Reptile/Invertebrate/Plant)
comdare_add_test(test_v41_anatomy_base
    SOURCES test_v41_anatomy_base.cpp)
target_include_directories(test_v41_anatomy_base PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_base PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.A2 ExecutionEngine als Wurzel ueber AnatomyBase (Lebewesen vs Viren)
comdare_add_test(test_v41_execution_engine
    SOURCES test_v41_execution_engine.cpp)
target_include_directories(test_v41_execution_engine PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_execution_engine PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.B SearchAlgorithmPermutationEngine (genus-aware Specialization)
comdare_add_test(test_v41_search_algorithm_permutation_engine
    SOURCES test_v41_search_algorithm_permutation_engine.cpp)
target_include_directories(test_v41_search_algorithm_permutation_engine PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_search_algorithm_permutation_engine PRIVATE Boost::mp11)

# V41.F.6.1.R5.D Anatomy Module ABI v1 (extern "C" Factory + Macro-Expansion)
comdare_add_test(test_v41_anatomy_module_abi
    SOURCES test_v41_anatomy_module_abi.cpp)
target_include_directories(test_v41_anatomy_module_abi PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
        # V41.P2 (2026-05-31) Umstufung-B im Mess-Pfad: ALLE 6 named ORGAN-Compositions als Mess-DLLs
        # (search_algo = sezierte Observable-Organe, direktiven-konform — Doku 14 §3.1). Jede Composition
        # traegt organ-kompatible Begleit-Achsen (anders als der AdHoc-Default-Pfad, der nur Monolithen trug).
        "::comdare::cache_engine::compositions::ArtComposition|compositions/art_reference.hpp"
        "::comdare::cache_engine::compositions::HotComposition|compositions/hot_reference.hpp"
        "::comdare::cache_engine::compositions::StartComposition|compositions/start_reference.hpp"
        "::comdare::cache_engine::compositions::SurfComposition|compositions/surf_reference.hpp"
        "::comdare::cache_engine::compositions::WormholeComposition|compositions/wormhole_reference.hpp"
        "::comdare::cache_engine::compositions::MasstreeComposition|compositions/masstree_reference.hpp"
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
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware"
    "${CMAKE_BINARY_DIR}/generated/axes/simd"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension")
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
    "${CMAKE_BINARY_DIR}/generated/axes/layout")
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
    "${CMAKE_BINARY_DIR}/generated/axes/path_compression"
    "${CMAKE_BINARY_DIR}/generated/axes/node")
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
    "${CMAKE_BINARY_DIR}/generated/axes/prefetch_axis")
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
    "${CMAKE_BINARY_DIR}/generated/axes/telemetry_axis")  # V41.F.2: axis_11_telemetry → axes/telemetry_axis
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
    "${CMAKE_BINARY_DIR}/generated/axes/serialization_axis")
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
    "${CMAKE_BINARY_DIR}/generated/axes/value_handle_axis")
target_link_libraries(test_v41_axis_14_value_handle PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.3 axis_08 Concurrency (8 Wrappers, Goldstandard)
comdare_add_test(test_v41_axis_08_concurrency
    SOURCES test_v41_axis_08_concurrency.cpp)
target_include_directories(test_v41_axis_08_concurrency PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/concurrency_axis")
target_link_libraries(test_v41_axis_08_concurrency PRIVATE
    Boost::mp11)

# V41.F.6.1 F.6 Phase A: migrierte errno-style Status-Codes (status.hpp prt-art -> cache-engine)
comdare_add_test(test_v41_status_code
    SOURCES test_v41_status_code.cpp)
target_include_directories(test_v41_status_code PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# V41.F.6.1 F.6 Phase A: migrierte Serialisierungs-Primitive (signaling_bits prt-art -> cache-engine)
comdare_add_test(test_v41_serialization_primitives
    SOURCES test_v41_serialization_primitives.cpp)
target_include_directories(test_v41_serialization_primitives PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")

# V41.F.6.1 F.6 NEUE Achse axis_01_page_type (6 Pflicht-Seitentypen)
comdare_add_test(test_v41_axis_01_page_type
    SOURCES test_v41_axis_01_page_type.cpp)
target_include_directories(test_v41_axis_01_page_type PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/topics/nodes/axis_01_page_type")
target_link_libraries(test_v41_axis_01_page_type PRIVATE Boost::mp11)

# V41.E11 Plugin-Controller Pruefling-Registry
comdare_add_test(test_v41_pruefling_registry
    SOURCES test_v41_pruefling_registry.cpp)
target_include_directories(test_v41_pruefling_registry PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# V41.E11 Phase B — Pruefling-Integration-Tests (von comdare_pruefling.cmake je geladenem
# Pruefling registriert; Listen kommen aus dem Plugin-Controller-Load-Loop der Root-CMakeLists).
foreach(_pt_src IN LISTS COMDARE_PRUEFLING_TEST_SOURCES)
    get_filename_component(_pt_name "${_pt_src}" NAME_WE)
    comdare_add_test(${_pt_name} SOURCES "${_pt_src}")
    # Volle CE-Include-Roots: Pruefling-Slots inkludieren CE-Achsen-Header (topics/), die
    # Merge-Mechanik (anatomy/) + die generierten Achsen-Flags (Phase B Slot-Füllung).
    target_include_directories(${_pt_name} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${CMAKE_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS}
        ${COMDARE_PRUEFLING_INCLUDE_DIRS})
    target_link_libraries(${_pt_name} PRIVATE Boost::mp11)
    message(STATUS "comdare Plugin-Controller: Pruefling-Integration-Test ${_pt_name} registriert")
endforeach()

# V41.F.6.1.R7.5.e axis_filter (4 Wrappers + TYPED_TEST_SUITE Pattern aus axis_06_allocator)
comdare_add_test(test_v41_axis_filter
    SOURCES test_v41_axis_filter.cpp)
target_include_directories(test_v41_axis_filter PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/filter_axis")
target_link_libraries(test_v41_axis_filter PRIVATE
    Boost::mp11)

# V41 #43 s4 — SuRF-Filter-Organ (axis_filter/composable): Filter-Property-Tests (no-FN + FP-Rate + Range)
comdare_add_test(test_v41_axis_filter_surf_organ
    SOURCES test_v41_axis_filter_surf_organ.cpp)
target_include_directories(test_v41_axis_filter_surf_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/filter_axis")
target_link_libraries(test_v41_axis_filter_surf_organ PRIVATE
    Boost::mp11)

# V41 SuRF S2 Inkrement 1 — echtes succinct LOUDS-Sparse-Filter-Organ (no-FN + FP-Tunbarkeit + S2>=S1-Kreuzbeleg)
comdare_add_test(test_v41_axis_filter_surf_louds_organ
    SOURCES test_v41_axis_filter_surf_louds_organ.cpp)
target_include_directories(test_v41_axis_filter_surf_louds_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/filter_axis")
target_link_libraries(test_v41_axis_filter_surf_louds_organ PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.f axis_io (4 Wrappers + TYPED_TEST_SUITE Pattern)
comdare_add_test(test_v41_axis_io
    SOURCES test_v41_axis_io.cpp)
target_include_directories(test_v41_axis_io PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/io_dispatch")  # V41.F.2: axis_io → axes/io_dispatch migriert
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
    "${CMAKE_BINARY_DIR}/generated/axes/migration_policy")  # V41.F.2: axis_migration → axes/migration_policy
target_link_libraries(test_v41_axis_migration PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.h axis_01 IndexOrganization (4 Clustering-Wrappers + TYPED_TEST_SUITE)
comdare_add_test(test_v41_axis_01_index_organization
    SOURCES test_v41_axis_01_index_organization.cpp)
target_include_directories(test_v41_axis_01_index_organization PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/index_organization")
target_link_libraries(test_v41_axis_01_index_organization PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.i.2 axis_09 Haupt-CPU-ISA (4 Wrappers Amd64/Aarch64/RiscV/PowerPc)
# Braucht auch axis_09b + axis_12 includes (topic_hardware_config_set referenziert alle)
comdare_add_test(test_v41_axis_09_isa
    SOURCES test_v41_axis_09_isa.cpp)
target_include_directories(test_v41_axis_09_isa PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/simd"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware")
target_link_libraries(test_v41_axis_09_isa PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.j axis_09b SIMD-Extension Sub-Achse (8 Wrappers + TYPED_TEST_SUITE)
comdare_add_test(test_v41_axis_09b_simd_extension
    SOURCES test_v41_axis_09b_simd_extension.cpp)
target_include_directories(test_v41_axis_09b_simd_extension PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/simd"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware")
target_link_libraries(test_v41_axis_09b_simd_extension PRIVATE
    Boost::mp11)

# V41 R5.C.3 / #704 — Cross-Axis-Constraint ISA × SIMD-Extension (Permutationsraum-Korrektheit)
comdare_add_test(test_v41_r5c3_isa_simd_cross_constraint
    SOURCES test_v41_r5c3_isa_simd_cross_constraint.cpp)
target_include_directories(test_v41_r5c3_isa_simd_cross_constraint PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_BINARY_DIR}/generated/axes/simd"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    "${CMAKE_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware")
target_link_libraries(test_v41_r5c3_isa_simd_cross_constraint PRIVATE
    Boost::mp11)

# axis_12 braucht jetzt auch axis_09b Include-Path
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

# V41.F.6.1 F.5 — comdare_perms_pa Pilot: Pruefling-Stufe-2-Composition -> DLL (Plugin-Controller).
# MATERIALISIERT die prt-art-Composition als Permutations-Binary (Beweis "Binaries aus Rekombinationen").
# Nur aktiv wenn ein Pruefling geladen ist (COMDARE_PRUEFLING_INCLUDE_DIRS via COMDARE_CE_PRUEFLINGE)
# UND das Codegen-Tool bereits gebaut wurde (2-Pass-Build, wie R5.I).
if(COMDARE_PRUEFLING_INCLUDE_DIRS)
    set(_pa_snippet   "${CMAKE_BINARY_DIR}/generated/comdare_perms_pa_list.cmake")
    set(_pa_pilot_dir "${CMAKE_BINARY_DIR}/generated/comdare_perms_pa_modules")
    comdare_run_anatomy_codegen_tool(
        OUTPUT       "${_pa_snippet}"
        NO_KNOWN
        LIBRARY_TYPE SHARED
        EXTERNAL_COMPOSITIONS
            "prtart-demo|::comdare::prt_art::slots::PrtArtCompositionDemo|prt_art/slots/prt_art_composition_demo.hpp"
        STATUS_OUT   _pa_status)

    if(_pa_status STREQUAL "FOUND" AND EXISTS "${_pa_snippet}")
        include("${_pa_snippet}")
        comdare_codegen_anatomy_module_list(
            PILOT_PREFIX  "comdare_perms_pa_pilot"
            OUTPUT_DIR    "${_pa_pilot_dir}"
            LIBRARY_TYPE  ${COMDARE_PERMUTATION_LIBRARY_TYPE}
            COMPOSITIONS  ${COMDARE_PERMUTATION_COMPOSITIONS}
            TARGETS_OUT   _pa_pilot_targets)
        # prt-art-Include-Pfad auf die generierten DLL-Targets (Composition-Header liegt in prt-art).
        foreach(_pa_t IN LISTS _pa_pilot_targets)
            target_include_directories(${_pa_t} PRIVATE ${COMDARE_PRUEFLING_INCLUDE_DIRS})
        endforeach()
        message(STATUS
            "F.5 comdare_perms_pa: Pruefling-Stufe-2-DLL-Codegen ACTIVE (targets=${_pa_pilot_targets})")
    else()
        message(STATUS
            "F.5 comdare_perms_pa: SKIPPED (status=${_pa_status}; Tool zuerst bauen + reconfigure).")
    endif()

    # --- Stufe 3: comdare_perms_full_join — CE-Compositions + prt-art gemeinsam → DLLs ---
    # Vervollständigt die 3-Stufen-Target-Trilogie (#15): Stufe 1 = R5.I-Pilot (CE-only),
    # Stufe 2 = comdare_perms_pa (oben, prt-art ersetzt), Stufe 3 = full-join (CE + prt-art unioniert).
    set(_fj_snippet   "${CMAKE_BINARY_DIR}/generated/comdare_perms_full_join_list.cmake")
    set(_fj_pilot_dir "${CMAKE_BINARY_DIR}/generated/comdare_perms_full_join_modules")
    comdare_run_anatomy_codegen_tool(
        OUTPUT       "${_fj_snippet}"
        NAMES        "art,hot"
        LIBRARY_TYPE SHARED
        EXTERNAL_COMPOSITIONS
            "prtart-demo|::comdare::prt_art::slots::PrtArtCompositionDemo|prt_art/slots/prt_art_composition_demo.hpp"
        STATUS_OUT   _fj_status)

    if(_fj_status STREQUAL "FOUND" AND EXISTS "${_fj_snippet}")
        include("${_fj_snippet}")
        comdare_codegen_anatomy_module_list(
            PILOT_PREFIX  "comdare_perms_full_join_pilot"
            OUTPUT_DIR    "${_fj_pilot_dir}"
            LIBRARY_TYPE  ${COMDARE_PERMUTATION_LIBRARY_TYPE}
            COMPOSITIONS  ${COMDARE_PERMUTATION_COMPOSITIONS}
            TARGETS_OUT   _fj_pilot_targets)
        # prt-art-Include additiv auf alle Targets (CE-Targets brauchen ihn nicht, unschädlich).
        foreach(_fj_t IN LISTS _fj_pilot_targets)
            target_include_directories(${_fj_t} PRIVATE ${COMDARE_PRUEFLING_INCLUDE_DIRS})
        endforeach()
        message(STATUS
            "F.5 comdare_perms_full_join: Stufe-3-DLL-Codegen ACTIVE (targets=${_fj_pilot_targets})")
    endif()
endif()

# V41.F.6.1 F15 — Mess-Treiber-Test (Stufe A): lädt generierte Permutations-DLLs + misst+vergleicht.
# CE-only (art,hot) → NICHT auf Pruefling gegated. 2-Pass-Build wie R5.I (Tool zuerst bauen).
set(_f15_snippet   "${CMAKE_BINARY_DIR}/generated/f15_perm_list.cmake")
set(_f15_pilot_dir "${CMAKE_BINARY_DIR}/generated/f15_measurement_modules")
comdare_run_anatomy_codegen_tool(
    OUTPUT       "${_f15_snippet}"
    NAMES        "art,hot"
    LIBRARY_TYPE SHARED
    STATUS_OUT   _f15_status)

if(_f15_status STREQUAL "FOUND" AND EXISTS "${_f15_snippet}")
    include("${_f15_snippet}")
    comdare_codegen_anatomy_module_list(
        PILOT_PREFIX  "f15_measurement_pilot"
        OUTPUT_DIR    "${_f15_pilot_dir}"
        LIBRARY_TYPE  ${COMDARE_PERMUTATION_LIBRARY_TYPE}
        COMPOSITIONS  ${COMDARE_PERMUTATION_COMPOSITIONS}
        TARGETS_OUT   _f15_pilot_targets)

    comdare_add_test(test_v41_anatomy_f15_measurement
        SOURCES test_v41_anatomy_f15_measurement.cpp)
    target_include_directories(test_v41_anatomy_f15_measurement PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${CMAKE_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(test_v41_anatomy_f15_measurement PRIVATE
        Boost::mp11
        comdare::anatomy_module_loader)
    target_compile_definitions(test_v41_anatomy_f15_measurement PRIVATE
        "COMDARE_F15_MEASUREMENT_DIR=\"${_f15_pilot_dir}\"")
    add_dependencies(test_v41_anatomy_f15_measurement ${_f15_pilot_targets})
    message(STATUS "F15: measurement driver test ENABLED")
else()
    message(STATUS
        "F15: SKIPPED (status=${_f15_status}). Build comdare_anatomy_codegen_cli first, then re-configure.")
endif()

# V41.F.6.1 R5.G — COMDARE_DEFINE_ANATOMY_MODULE_ADHOC-Makro (Auto-Permutations-Materialisierung).
# COMDARE_ANATOMY_ABI_STATIC=1 → die vom Makro definierten extern-C-Symbole sind im Test-Exe plain
# definierbar + direkt aufrufbar (kein dllimport). CE-only, kein Codegen-Tool/2-Pass nötig.
comdare_add_test(test_v41_anatomy_adhoc_codegen_macro
    SOURCES test_v41_anatomy_adhoc_codegen_macro.cpp)
target_include_directories(test_v41_anatomy_adhoc_codegen_macro PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_adhoc_codegen_macro PRIVATE Boost::mp11)

# V41.F.6.1.R6 — Prüf-Dock SearchAlgorithm (per-Gattung ABI-Mess-Übergang, Doku 24 §8.8). In-Process-Test:
# verkabelt IObservableTier + AnatomyModuleHandle + drive_tier_observe_trace_abi hinter IPruefDock.
comdare_add_test(test_v41_pruef_dock_search_algorithm
    SOURCES test_v41_pruef_dock_search_algorithm.cpp)
target_include_directories(test_v41_pruef_dock_search_algorithm PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_pruef_dock_search_algorithm PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
target_compile_definitions(test_v41_anatomy_adhoc_codegen_macro PRIVATE COMDARE_ANATOMY_ABI_STATIC=1)

# V5-I4 — Konformitäts-Gate (std::map-Hüllen-Konformität je Gattung VOR Messung; header-only + std, kein Link).
comdare_add_test(test_v5_conformance_gate
    SOURCES test_v5_conformance_gate.cpp)
target_include_directories(test_v5_conformance_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")

# V5-I5 — MementoAxis-Concept + save_axis/restore_axis Round-Trip (Fundament memento_all; header-only + std).
comdare_add_test(test_v5_memento_axis
    SOURCES test_v5_memento_axis.cpp)
target_include_directories(test_v5_memento_axis PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})

# V5-I7 — Zwei-Phasen-Treiber: Invariante 2phase==1phase==cold-fallback (header-only + std, std::map-Mock-Tier).
comdare_add_test(test_v5_two_phase_driver
    SOURCES test_v5_two_phase_driver.cpp)
target_include_directories(test_v5_two_phase_driver PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")

# V5-I9 — WorkloadOrchestrator: host-seitiger Lastprofil-Treiber + Multi-Profil + Serialisierung (linkt Generator).
comdare_add_test(test_v5_workload_orchestrator
    SOURCES test_v5_workload_orchestrator.cpp)
target_include_directories(test_v5_workload_orchestrator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_v5_workload_orchestrator PRIVATE
    comdare::workload_driver)

# V5-I1-SUBSTANZ (#50) — ComdareMeasurementSnapshotV1: EIN autoritativer 16+6-Mess-POD + kanonische Serializer.
comdare_add_test(test_v5_measurement_snapshot
    SOURCES test_v5_measurement_snapshot.cpp)
target_include_directories(test_v5_measurement_snapshot PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_v5_measurement_snapshot PRIVATE
    comdare::workload_driver)

# V5-#46-REFERENZ — Disk-persistierender MementoAxis (Beweis "Memento inkl. IO/Disk-Persistenz").
comdare_add_test(test_v5_disk_memento
    SOURCES test_v5_disk_memento.cpp)
target_include_directories(test_v5_disk_memento PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# V5-#49 Pfad A — YCSB-Treue: Zipfian/Latest-Key-Verteilung (Definitionsmerkmal) statt uniform.
comdare_add_test(test_v5_ycsb_distribution
    SOURCES test_v5_ycsb_distribution.cpp)
target_include_directories(test_v5_ycsb_distribution PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_v5_ycsb_distribution PRIVATE comdare::workload_driver)

# V5-#49-E/F — YCSB-Op-Set: Range-Scan (E, via IScannableTier) + Read-Modify-Write (F, host-seitig).
comdare_add_test(test_v5_ycsb_op_set
    SOURCES test_v5_ycsb_op_set.cpp)
target_include_directories(test_v5_ycsb_op_set PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_v5_ycsb_op_set PRIVATE comdare::workload_driver)

# V5-I6-SUBSTANZ (#44) — echte produktive MementoAxis auf den Such-Organen (ComposedSearch + Observable-Wrapper).
comdare_add_test(test_v5_organ_memento
    SOURCES test_v5_organ_memento.cpp)
target_include_directories(test_v5_organ_memento PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v5_organ_memento PRIVATE Boost::mp11)

# V41.F.6.1 R5.G — constexpr type_name<T>() (Auto-Emitter-Grundlage: FQ-Typ-Name als String).
comdare_add_test(test_v41_codegen_type_name
    SOURCES test_v41_codegen_type_name.cpp)
target_include_directories(test_v41_codegen_type_name PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_codegen_type_name PRIVATE Boost::mp11)

# V41.F.4 — ICacheEngineTools Tools-Plugin-Facade: Smoke-Test (Statistik/Codegen/Workload/External-Plugin).
comdare_add_test(test_v41_cache_engine_tools_facade
    SOURCES test_v41_cache_engine_tools_facade.cpp
    LIBRARIES comdare::workload_generator)
target_include_directories(test_v41_cache_engine_tools_facade PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# V41.F.6.1 R5.G — all_axes_umbrella.hpp (Auto-Emitter-Plumbing: alle 17 Achsen in einem Include).
comdare_add_test(test_v41_codegen_all_axes_umbrella
    SOURCES test_v41_codegen_all_axes_umbrella.cpp)
target_include_directories(test_v41_codegen_all_axes_umbrella PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_codegen_all_axes_umbrella PRIVATE Boost::mp11)

# V41.F.6.1 R5.G — End-to-End: auto-emittiertes AdHoc-Permutations-Modul → SHARED-DLL → laden + messen.
set(_r5g_adhoc_dir "${CMAKE_BINARY_DIR}/generated/r5g_adhoc_modules")
add_library(comdare_r5g_adhoc_perm SHARED auto_emitted_perm_module.cpp)
target_include_directories(comdare_r5g_adhoc_perm PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(comdare_r5g_adhoc_perm PRIVATE Boost::mp11)
target_compile_features(comdare_r5g_adhoc_perm PRIVATE cxx_std_23)
target_compile_definitions(comdare_r5g_adhoc_perm PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
set_target_properties(comdare_r5g_adhoc_perm PROPERTIES
    POSITION_INDEPENDENT_CODE                ON
    OUTPUT_NAME                              "comdare_anatomy_perm_r5g_adhoc"
    RUNTIME_OUTPUT_DIRECTORY                 "${_r5g_adhoc_dir}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${_r5g_adhoc_dir}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${_r5g_adhoc_dir}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_r5g_adhoc_dir}"
    LIBRARY_OUTPUT_DIRECTORY                 "${_r5g_adhoc_dir}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE         "${_r5g_adhoc_dir}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG           "${_r5g_adhoc_dir}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_r5g_adhoc_dir}")
# V42 L-74c: _DEBUG/_RELWITHDEBINFO-Output-Dirs explizit → die codegen-DLL landet bei JEDER Build-Config
# direkt im Lade-Ort (COMDARE_R5G_ADHOC_DLL_DIR), nicht im Multi-Config-Debug-Unterordner. Behebt den
# „alte DLL geladen / neue DLL im Debug-Subdir"-Bug (Befund 2026-06-03: V2-DLL-Round-Trip lud sonst die
# veraltete DLL → tier_observe_v2-SKIP statt echtem V2-POD).
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_r5g_adhoc_perm)
endif()

comdare_add_test(test_v41_anatomy_adhoc_dll_load
    SOURCES test_v41_anatomy_adhoc_dll_load.cpp)
target_include_directories(test_v41_anatomy_adhoc_dll_load PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_v41_anatomy_adhoc_dll_load PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
target_compile_definitions(test_v41_anatomy_adhoc_dll_load PRIVATE
    "COMDARE_R5G_ADHOC_DLL_DIR=\"${_r5g_adhoc_dir}\"")
add_dependencies(test_v41_anatomy_adhoc_dll_load comdare_r5g_adhoc_perm)

# V41.F.6.1 R5.G — SKALIERUNG: comdare-adhoc-emitter enumeriert den GANZEN Pilot-Raum (Configure-Time)
# und CMake baut JEDE emittierte Permutation zu einer eigenen SHARED-DLL. 2-Pass:
#   Pass 1: cmake -B build  (Tool noch nicht gebaut -> SKIPPED, Tool-Target eingehaengt)
#   Build:  cmake --build build --target comdare_adhoc_emitter_cli
#   Pass 2: cmake -B build  -> Emitter laeuft, DLLs + Load-Test werden gebaut
include(adhoc_emitter)
set(_r5g_autobuilt_dir "${CMAKE_BINARY_DIR}/generated/r5g_autobuilt_modules")
# V41.F.6.1 R5.D — optionaler Voll-Coverage-Modus: statt des kuratierten 48er-Pilot-Raums die
# 1-wise-Ueberdeckungs-Stichprobe (search 17 × allocator 25 × layout 5 → 25 Permutationen, jede
# Variante mind. 1×) als DLLs bauen. Default OFF → keine Regression / kein Build-Zeit-Aufschlag.
option(COMDARE_CE_ADHOC_FULL_COVERAGE
    "R5.D: adhoc-emitter im --full-coverage-Modus (25 1-wise-DLLs statt 48er-Pilot)" OFF)
if(COMDARE_CE_ADHOC_FULL_COVERAGE)
    comdare_run_adhoc_emitter(
        OUTPUT_DIR "${_r5g_autobuilt_dir}"
        STATUS_OUT _r5g_ae_status
        FULL_COVERAGE)
else()
    comdare_run_adhoc_emitter(
        OUTPUT_DIR "${_r5g_autobuilt_dir}"
        STATUS_OUT _r5g_ae_status)
endif()
if(_r5g_ae_status STREQUAL "FOUND")
    comdare_build_adhoc_modules(
        OUTPUT_DIR    "${_r5g_autobuilt_dir}"
        PILOT_PREFIX  "comdare_r5g_autobuilt_perm"
        AXIS_GEN_DIRS ${COMDARE_ALL_AXIS_GENERATED_DIRS}
        TARGETS_OUT   _r5g_autobuilt_targets)
    list(LENGTH _r5g_autobuilt_targets _r5g_autobuilt_count)
    if(_r5g_autobuilt_count GREATER 0)
        comdare_add_test(test_v41_anatomy_adhoc_autobuilt_load
            SOURCES test_v41_anatomy_adhoc_autobuilt_load.cpp)
        target_include_directories(test_v41_anatomy_adhoc_autobuilt_load PRIVATE
            "${PROJECT_SOURCE_DIR}/libs/cache_engine"
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
            "${CMAKE_BINARY_DIR}/generated")
        target_link_libraries(test_v41_anatomy_adhoc_autobuilt_load PRIVATE
            Boost::mp11
            comdare::anatomy_module_loader)
        target_compile_definitions(test_v41_anatomy_adhoc_autobuilt_load PRIVATE
            "COMDARE_R5G_AUTOBUILT_DLL_DIR=\"${_r5g_autobuilt_dir}\""
            "COMDARE_R5G_AUTOBUILT_COUNT=${_r5g_autobuilt_count}")
        add_dependencies(test_v41_anatomy_adhoc_autobuilt_load ${_r5g_autobuilt_targets})
        message(STATUS
            "R5.G: Auto-Permutations-Skalierung ACTIVE (${_r5g_autobuilt_count} auto-gebaute DLLs).")

        # V41.F.6.1 R5.D — Smoke-Test des comdare-f15-compare-CLI ueber die realen R5.G-DLLs:
        # faehrt die echte Binary (nicht nur die Treiber-Logik) + prueft Exit-0 + "win_rate"-Ausgabe.
        # (apps/ wird vor tests/ verarbeitet → Target existiert; default-Build baut CLI + DLLs.)
        if(TARGET comdare_f15_compare_cli)
            add_test(NAME f15_compare_cli_smoke
                COMMAND comdare_f15_compare_cli "${_r5g_autobuilt_dir}" --batches=16 --alpha=0.05)
            set_tests_properties(f15_compare_cli_smoke PROPERTIES
                PASS_REGULAR_EXPRESSION "Ranking .schnellste"
                LABELS "r5d;f15;cli")
        endif()
    endif()
else()
    message(STATUS
        "R5.G: Auto-Permutations-Skalierung SKIPPED (status=${_r5g_ae_status}). "
        "Build comdare_adhoc_emitter_cli first, then re-configure.")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Goal-V6 Phase D — standalone main()-Tests (KEIN gtest; eigenes main + errno-style return).
# Behebt den Phase-E-Audit-Vorbehalt: vorher nur via git-ignorierte build/scratch_*.ps1 verifiziert
# (autor-behauptet, nicht repo-reproduzierbar) → jetzt CTest-registriert (ctest baut + laeuft sie).
# add_test direkt (nicht comdare_add_test, das ist gtest). Include-Roots = die scratch-verifizierten.
# ─────────────────────────────────────────────────────────────────────────────
# (a) leichtgewichtig: nur libs/cache_engine(+include/common); kein Boost/generated.
set(COMDARE_GOALV6_LIGHT_DTESTS
    test_d1_d2_selection      # L-SEL + L-73 (BuildSelection + provision_all O(K))
    test_d3_ceb_generator     # L-77 (ceb_generator generate_all_real)
    test_d7_build_variant     # L-74b (BuildVariantDefinition)
    test_d7a_build_inspection # L-74a (Build-Variant-Inspection-ABI)
    test_d7b_definition_per_node # L-74b (NodeValue build_def + build_def_real + 22-Vollstaendigkeit)
    test_d8_operability       # L-74c (R5.B-Operabilitaets-Klassifikation)
    test_d9_set               # L-76a (Set-Gattung Kern)
    test_d9_set_adapter       # L-76a (SetAbiAdapter)
    test_d10_sequence         # L-76b (Sequence-Gattung)
    test_d11_view             # L-76c (View-Gattung)
    test_genus_docks          # L-76a-c (Set/Sequence/View Pruef-Docks: Gattungs-API treiben + Observer + CSV)
    test_d12_virus            # L-76d (Viren IVirusExecutionEngine + GraphBfs)
    test_d13_runtime_measure  # L-MEAS (RuntimeMeasureVisitor)
    test_d14_result_ingest    # L-CLUSTER gate-frei (result_ingest)
    test_d14b_perm_runner     # L-CLUSTER-E2E gate-frei (perm_runner → result_ingest Round-Trip)
    test_d14c_e2e_pipeline)   # L-CLUSTER-E2E gate-frei (durchgängige Pipeline, O(K))
foreach(_dt IN LISTS COMDARE_GOALV6_LIGHT_DTESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common")
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "goal_v6;phase_d")
endforeach()

# (b) Container-Gattungs-Tests: brauchen die q1/q2-Achsen-Registries → Boost::mp11 + generated-Flags.
set(COMDARE_GOALV6_BOOST_DTESTS
    test_container_genus            # #75 Container q2-Slot (in-process)
    test_container_dock             # #75 Container-Pruef-Dock
    test_d_v42_probe                # V42-Composition-Driver-Probe (telemetry default_ctor/ObservableAxis-Befund)
    test_d_v42_probe2               # V42-Korrektur-Probe (LeafOnlyCounter: is_aggregate/{}-Block/default-init)
    test_d_v42_telemetry_observable  # V42 L-74c ObservableTelemetry-Driver-Baustein (setzt STATISTICS in-Quelle)
    test_d_v42_memory_layout_observable  # V42 L-74c ObservableMemoryLayout-Driver-Baustein (static+observe_scan)
    test_d_v42_serialization_observable  # V42 L-74c ObservableSerialization-Driver-Baustein
    test_d_v42_node_type_observable      # V42 L-74c ObservableNodeType-Driver-Baustein (N in ComposedStore)
    test_d_v42_telemetry_registry_observable  # V42 L-74c telemetry-Registry-Pfad (StaticAxisVariants -> Huellen)
    test_genus_permutation_engines            # L-76a-c per-Gattung PermutationEngines (Set/Sequence/View Factory+Engine)
    test_axis_growth_policies)                # L-76b axis_growth Mehr-Policy-Achse (Doubling/GoldenRatio/FixedChunk/Exact + Registry)
foreach(_dt IN LISTS COMDARE_GOALV6_BOOST_DTESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${CMAKE_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_dt} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "goal_v6;phase_d")
endforeach()

# ─────────────────────────────────────────────────────────────────────────────
# Goal-V6 L-76/L-74a — DLL-Round-Trips als OFFIZIELLE CMake-Targets (vorher nur committete PS1-Skripte). Die 5
# boost-freien Permutations-/Build-Varianten-DLLs (interne Organe) + ihre Loader-Tests. Die DLL-Pfade gehen via
# $<TARGET_FILE:..> als add_test-Argumente → kein Lade-Ort-Rätselraten (Multi-Config-robust). C++23.
# ─────────────────────────────────────────────────────────────────────────────
set(_goalv6_dll_dir "${CMAKE_BINARY_DIR}/generated/goalv6_dll_modules")
set(_src_perm_set_d9               genus_module_set.cpp)
set(_src_perm_sequence_d10         genus_module_sequence.cpp)
set(_src_perm_view_d11             genus_module_view.cpp)
set(_src_perm_buildvariant_avx512  genus_buildvariant_avx512.cpp)
set(_src_perm_buildvariant_avx2    genus_buildvariant_avx2.cpp)
foreach(_dll IN ITEMS perm_set_d9 perm_sequence_d10 perm_view_d11 perm_buildvariant_avx512 perm_buildvariant_avx2)
    add_library(${_dll} SHARED "${_src_${_dll}}")
    target_include_directories(${_dll} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common")
    target_compile_features(${_dll} PRIVATE cxx_std_23)
    target_compile_definitions(${_dll} PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
    set_target_properties(${_dll} PROPERTIES
        POSITION_INDEPENDENT_CODE                ON
        RUNTIME_OUTPUT_DIRECTORY                 "${_goalv6_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${_goalv6_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${_goalv6_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY                 "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE         "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG           "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_goalv6_dll_dir}")
    if(COMMAND comdare_set_platform_defines)
        comdare_set_platform_defines(${_dll})
    endif()
endforeach()

# Genus-DLL-Loader-Test (gattungs-agnostischer Loader → ISetTier/ISequenceTier/IViewTier über die .dll-Grenze).
add_executable(test_dgenus_dll test_dgenus_dll.cpp)
target_include_directories(test_dgenus_dll PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine" "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_features(test_dgenus_dll PRIVATE cxx_std_23)
target_link_libraries(test_dgenus_dll PRIVATE comdare::anatomy_module_loader)
add_dependencies(test_dgenus_dll perm_set_d9 perm_sequence_d10 perm_view_d11)
add_test(NAME test_dgenus_dll COMMAND test_dgenus_dll
    "$<TARGET_FILE:perm_set_d9>" "$<TARGET_FILE:perm_sequence_d10>" "$<TARGET_FILE:perm_view_d11>")
set_tests_properties(test_dgenus_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

# Build-Varianten-DLL-Loader-Test (L-74a, win32 LoadLibrary/GetProcAddress — Avx512 vs Avx2 über die .dll-Grenze).
add_executable(test_buildvariant_dll test_buildvariant_dll.cpp)
target_include_directories(test_buildvariant_dll PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine" "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_features(test_buildvariant_dll PRIVATE cxx_std_23)
add_dependencies(test_buildvariant_dll perm_buildvariant_avx512 perm_buildvariant_avx2)
add_test(NAME test_buildvariant_dll COMMAND test_buildvariant_dll
    "$<TARGET_FILE:perm_buildvariant_avx512>" "$<TARGET_FILE:perm_buildvariant_avx2>")
set_tests_properties(test_buildvariant_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

# L-74a — Build-Varianten-DLLs mit den ECHTEN axis_01/09b/12-Wrapper-KLASSEN (DenseBytePageType/Avx512SimdExtension/
# X86_64HardwareProfile) statt real-geformter Stubs. Brauchen die generierten flags.hpp-Dirs + Boost::mp11 → eigene
# Registrierung (nicht in der boost-freien Schleife oben). Beweist: die REALEN Library-Wrapper sind DLL-einbaubar.
set(_rsrc_perm_buildvariant_real_avx512 genus_buildvariant_real_avx512.cpp)
set(_rsrc_perm_buildvariant_real_avx2   genus_buildvariant_real_avx2.cpp)
foreach(_rdll IN ITEMS perm_buildvariant_real_avx512 perm_buildvariant_real_avx2)
    add_library(${_rdll} SHARED "${_rsrc_${_rdll}}")
    target_include_directories(${_rdll} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${CMAKE_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_rdll} PRIVATE Boost::mp11)
    target_compile_features(${_rdll} PRIVATE cxx_std_23)
    target_compile_definitions(${_rdll} PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
    set_target_properties(${_rdll} PROPERTIES
        POSITION_INDEPENDENT_CODE                ON
        RUNTIME_OUTPUT_DIRECTORY                 "${_goalv6_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${_goalv6_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${_goalv6_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY                 "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE         "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG           "${_goalv6_dll_dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_goalv6_dll_dir}")
    if(COMMAND comdare_set_platform_defines)
        comdare_set_platform_defines(${_rdll})
    endif()
endforeach()
add_dependencies(test_buildvariant_dll perm_buildvariant_real_avx512 perm_buildvariant_real_avx2)
add_test(NAME test_buildvariant_dll_real COMMAND test_buildvariant_dll
    "$<TARGET_FILE:perm_buildvariant_real_avx512>" "$<TARGET_FILE:perm_buildvariant_real_avx2>")
set_tests_properties(test_buildvariant_dll_real PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

# L-74a — ADHOC-BUILDVARIANT-DLL: EINE DLL traegt die 17-Slot-Anatomie (4 ABI-Symbole) UND die 3-Build-Achsen-
# Inspection (comdare_build_variant_inspect). Heavy (Anatomie-Umbrella + Boost + generierte Dirs, wie comdare_r5g).
add_library(perm_adhoc_buildvariant SHARED genus_adhoc_buildvariant.cpp)
target_include_directories(perm_adhoc_buildvariant PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(perm_adhoc_buildvariant PRIVATE Boost::mp11)
target_compile_features(perm_adhoc_buildvariant PRIVATE cxx_std_23)
target_compile_definitions(perm_adhoc_buildvariant PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
set_target_properties(perm_adhoc_buildvariant PROPERTIES
    POSITION_INDEPENDENT_CODE                ON
    RUNTIME_OUTPUT_DIRECTORY                 "${_goalv6_dll_dir}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${_goalv6_dll_dir}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${_goalv6_dll_dir}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_goalv6_dll_dir}"
    LIBRARY_OUTPUT_DIRECTORY                 "${_goalv6_dll_dir}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE         "${_goalv6_dll_dir}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG           "${_goalv6_dll_dir}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_goalv6_dll_dir}")
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(perm_adhoc_buildvariant)
endif()

add_executable(test_adhoc_buildvariant_dll test_adhoc_buildvariant_dll.cpp)
target_include_directories(test_adhoc_buildvariant_dll PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine" "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_features(test_adhoc_buildvariant_dll PRIVATE cxx_std_23)
target_link_libraries(test_adhoc_buildvariant_dll PRIVATE comdare::anatomy_module_loader)
add_dependencies(test_adhoc_buildvariant_dll perm_adhoc_buildvariant)
add_test(NAME test_adhoc_buildvariant_dll COMMAND test_adhoc_buildvariant_dll "$<TARGET_FILE:perm_adhoc_buildvariant>")
set_tests_properties(test_adhoc_buildvariant_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

message(STATUS "Goal-V6 Phase D: ${CMAKE_CURRENT_LIST_LINE} — 27 add_test-D-Tests (16 leicht + 11 Boost) + 4 DLL-Round-Trip-Tests "
    "(test_dgenus_dll/test_buildvariant_dll/test_buildvariant_dll_real/test_adhoc_buildvariant_dll) als OFFIZIELLE CMake-Targets (6 SHARED-DLLs via $<TARGET_FILE>). "
    "container_dll (test_d4b) bleibt scratch-verifiziert (Folge).")
