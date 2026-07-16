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
            "${PROJECT_BINARY_DIR}/generated/topics/${_axis_rel}")
    endif()
endforeach()
# V41.F.2: physisch nach axes/<axis>/ migrierte Achsen (Glob topics/*/axis_* findet sie nicht) — generisch
# je migriertem axes/<axis>-Verzeichnis das generated-Dir ergaenzen, sonst C1083 in umbrella/compositions.
file(GLOB _comdare_axes_dirs LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_axes_dir ${_comdare_axes_dirs})
    if(IS_DIRECTORY "${_axes_dir}")
        get_filename_component(_axes_name "${_axes_dir}" NAME)
        list(APPEND COMDARE_ALL_AXIS_GENERATED_DIRS "${PROJECT_BINARY_DIR}/generated/axes/${_axes_name}")
    endif()
endforeach()

# INC-F (2026-07-14): den generierten axis_*_flags-Include-Satz als GLOBAL-Property exportieren, damit der
# super-Subbuild-seitige W4-Gate-Test (super/Code/02_messung_driver/test_v32_katalog_driver, opt-in
# COMDARE_V32_DRIVER_ENABLE) ihn via get_property liest — SINGLE-SOURCE (kein hartcodierter Dir-Satz, kein
# Drift bei neuen Achsen). Laeuft im ce-Standalone-Build harmlos leer mit; im super-Embed (COMDARE_BUILD_TESTS
# ON) wird ce VOR 02_messung_driver konfiguriert -> die Property ist dort verfuegbar. PROJECT_BINARY_DIR ist im
# Embed == ${CMAKE_BINARY_DIR}/_cache_engine_external -> die Pfade stimmen fuer beide Builds.
set_property(GLOBAL PROPERTY COMDARE_CE_AXIS_GENERATED_DIRS "${COMDARE_ALL_AXIS_GENERATED_DIRS}")

function(comdare_prepare_thesis_object_dir target)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target}.dir/thesis_tiere")
endfunction()

# Header-only INTERFACE-Library: Cache-Engine Concepts
add_library(comdare_cache_engine_concepts INTERFACE)
add_library(comdare::cache_engine_concepts ALIAS comdare_cache_engine_concepts)
target_include_directories(comdare_cache_engine_concepts INTERFACE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/subsystems")
target_compile_features(comdare_cache_engine_concepts INTERFACE cxx_std_23)
comdare_set_platform_defines(comdare_cache_engine_concepts)

# Header-only INTERFACE-Library: PRT-ART Concepts
add_library(comdare_prt_art_concepts INTERFACE)
add_library(comdare::prt_art_concepts ALIAS comdare_prt_art_concepts)
target_include_directories(comdare_prt_art_concepts INTERFACE
    "${PROJECT_SOURCE_DIR}/prt_art/include")
target_compile_features(comdare_prt_art_concepts INTERFACE cxx_std_23)
target_link_libraries(comdare_prt_art_concepts INTERFACE comdare::cache_engine_concepts)
comdare_set_platform_defines(comdare_prt_art_concepts)

# V41.F.6.1 E1 (2026-05-29) — Legacy-prt_art-Concept-Tests gaten: sie inkludieren
# prt_art/concepts|page_structures-Header, die in der F.6-Migration nach cache-engine-Achsen
# verschoben wurden und im STANDALONE-Build (ohne befuelltes prt-art-Submodul) NICHT vorhanden sind
# → wuerden mit C1083 als _NOT_BUILT scheitern. Root-cause-korrekt: nur registrieren, wenn die
# prt-art-Legacy-Header tatsaechlich da sind (mit COMDARE_CE_PRUEFLINGE/prt-art-Kontext bauen sie).
set(COMDARE_PRT_ART_LEGACY_AVAILABLE FALSE)
if(EXISTS "${PROJECT_SOURCE_DIR}/prt_art/include/prt_art/prt_art.hpp")
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
    # #265-b: die AP-3-Bruecke konsumiert die vendored foundation-Zelle comdare::platform (SIMDDetector).
    LIBRARIES comdare::cache_engine_concepts comdare::platform)

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

# #195 contract config-durability-Gate (Pipeline P0, EPIC #186): Müll-/kaputte Config-XMLs
# duerfen die Parser (cx::parse_document + wd::parse_load_profile) NICHT crashen (Segfault/UB),
# sondern muessen sauber nullopt liefern. GTest-Negativtest als hartes CI-Gate
# (.gitlab-ci.yml: Job contract:durability). Header-only Parser → keine LIBRARIES noetig;
# Include-Roots gespiegelt von test_a1_load_profile_opmix (load_profile_parser.hpp + transitiv
# serialization/xml_config_parser/xml_reader.hpp).
comdare_add_test(test_config_durability
    SOURCES test_config_durability.cpp)
target_include_directories(test_config_durability PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")

# #197 chaos drift-gate (Pipeline P0, EPIC #186, Stufe 6): Mess-Drift-Detektor + Rerun-Gate.
# Deterministischer Mechanismus-Test (Stage `chaos`, Job chaos:drift in .gitlab-ci.yml): assess_drift
# klassifiziert stabile/instabile Wiederholungs-Gruppen + run_with_drift_gate heilt sich selbst (Rerun
# + Warn-Log) bzw. gibt advisory die stabilste Gruppe zurueck. Header-only
# (builder/commands/drift_detector.hpp → builder/commands/latency_stats.hpp) → keine LIBRARIES; Include-
# Root libs/cache_engine wie #195. (Anwendung auf reale PMC-Messungen + Schwellen-Kalibrierung = #156.)
comdare_add_test(test_chaos_drift_gate
    SOURCES test_chaos_drift_gate.cpp)
target_include_directories(test_chaos_drift_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

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

comdare_add_test(test_permutation_sampling
    SOURCES test_permutation_sampling.cpp)
target_include_directories(test_permutation_sampling PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")

# Phase 8 (2026-05-13): LaTeX-Anhang-Tool
comdare_add_test(test_latex_anhang
    SOURCES test_latex_anhang.cpp
    LIBRARIES comdare::latex_anhang_lib)

# Aufgabe #104 (2026-05-13): eigene RCU-Implementation
comdare_add_test(test_rcu
    SOURCES test_rcu.cpp
    LIBRARIES comdare::rcu)

# Feature #25 (2026-07-13): Eigentumsmodell-/Concurrency-Tests fuer die RCU-Reader-Registry
# (Ownership-Inversion: Thread-Ende + Stack-Domain-Adress-Wiederverwendung, ehemals dangling).
comdare_add_test(test_rcu_concurrency
    SOURCES test_rcu_concurrency.cpp
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
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
set_target_properties(comdare_perm_mock_test PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    # #278 A3 (2026-07-06, Job 213964): load_all filtert auf das comdare_perm_-Praefix (Mess-Pfad-Konvention;
    # der echte Emitter baut praefixfrei). CMakes lib-Praefix machte die Mock-DLL zu libcomdare_perm_...so
    # -> Verzeichnis-Scan fand 0 Kandidaten. PREFIX "" = Mock folgt der Konvention (Filter bleibt strikt).
    PREFIX "")

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
target_include_directories(test_v31_adapters PRIVATE "${PROJECT_SOURCE_DIR}")

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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_topic_allocator_axis_06 PRIVATE Boost::mp11)
# Audit #212/K5b (2026-06-28): die drei Topic-Observer-Mechanik-Tests pruefen den per-Achsen-PUSH
# (observer().on_event/notify/has_callback). Der Push ist projektweit Default-OFF (zero-cost NullNotify,
# s. Root-CMakeLists.txt COMDARE_CE_ENABLE_OBSERVER_PUSH); diese Targets schalten ihn lokal AN.
target_compile_definitions(test_v41_topic_allocator_axis_06 PRIVATE COMDARE_CE_ENABLE_OBSERVER_PUSH)

# V41.F.6.1 Topic queuing Pilot (2026-05-26)
comdare_add_test(test_v41_topic_queuing
    SOURCES test_v41_topic_queuing.cpp)
target_include_directories(test_v41_topic_queuing PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_topic_queuing PRIVATE Boost::mp11)
target_compile_definitions(test_v41_topic_queuing PRIVATE COMDARE_CE_ENABLE_OBSERVER_PUSH)  # #212/K5b: s.o.

# V41.F.6.1 Topic traversal Pilot (2026-05-26)
comdare_add_test(test_v41_topic_traversal
    SOURCES test_v41_topic_traversal.cpp)
target_include_directories(test_v41_topic_traversal PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    # V41 Saeule-1 (Doku 24 §6): der traversal-Test komponiert das Traversal-Organ mit dem
    # nodes-axis_04 Storage-Organ (NodeTypeSlotStore<Node4NodeType>) -> braucht die generierten
    # Flags aller Achsen (wie compositions/anatomy-Tests). Folge-Increments (layout/allocator) inklusive.
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_topic_traversal PRIVATE Boost::mp11)
target_compile_definitions(test_v41_topic_traversal PRIVATE COMDARE_CE_ENABLE_OBSERVER_PUSH)  # #212/K5b: s.o.

# V41.F.6.1.P1 Phase B Paper-Legacy-Code Pattern (ctsha + Sub-Concept + Macro)
comdare_add_test(test_v41_paper_legacy_code
    SOURCES test_v41_paper_legacy_code.cpp)
target_include_directories(test_v41_paper_legacy_code PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_paper_legacy_code PRIVATE Boost::mp11)

# V41.F.6.1.R2 Composition-Templates (Architektur-Refactoring nach Organ-Metapher)
comdare_add_test(test_v41_compositions
    SOURCES test_v41_compositions.cpp)
target_include_directories(test_v41_compositions PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_compositions PRIVATE Boost::mp11)

# V41.F.6.1.F Topics-Fundament-Sprint (12 Topics Skelett-Stufe-A)
comdare_add_test(test_v41_topics_fundament
    SOURCES test_v41_topics_fundament.cpp)
target_include_directories(test_v41_topics_fundament PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_topics_fundament PRIVATE Boost::mp11)

# V41.F.6.1.R3 SearchAlgorithmAnatomy Skelett (Saeugetier-Anatomie)
comdare_add_test(test_v41_anatomy
    SOURCES test_v41_anatomy.cpp)
target_include_directories(test_v41_anatomy PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy PRIVATE Boost::mp11)

# V41.F.6.1.R4 AnatomyPermutationDriver (PermutationEngine + Anatomie-Brueckenkopf)
comdare_add_test(test_v41_anatomy_r4_driver
    SOURCES test_v41_anatomy_r4_driver.cpp)
target_include_directories(test_v41_anatomy_r4_driver PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_r4_driver PRIVATE Boost::mp11)

# V41.F.6.1.R5.A ObserverAggregate (ABI-stabiler 17-Achsen-Snapshot)
comdare_add_test(test_v41_anatomy_observer
    SOURCES test_v41_anatomy_observer.cpp)
target_include_directories(test_v41_anatomy_observer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_observer PRIVATE Boost::mp11)

# V41 Roadmap-3 (Doku 24 §2.1/§2.2) Saeule-2 Mess-Pfad: TierObserveTrace Fuellstands-Treiber
comdare_add_test(test_v41_tier_observe_trace
    SOURCES test_v41_tier_observe_trace.cpp)
target_include_directories(test_v41_tier_observe_trace PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_03a_cross_variant_equivalence PRIVATE Boost::mp11)

# AP-7a/#241 -- S22 SwissTableSearchAlgo: Wrapper-Konformitaet + Scalar-Gruppen-Probe.
comdare_add_test(test_ap7_swisstable
    SOURCES test_ap7_swisstable.cpp)
target_include_directories(test_ap7_swisstable PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap7_swisstable PRIVATE Boost::mp11)
# AP-7b/#262 -- S22 SwissTableOrgan: native Weg-B Organ + DEG-1 harvest.
comdare_add_test(test_ap7b_swiss_organ
    SOURCES test_ap7b_swiss_organ.cpp)
target_include_directories(test_ap7b_swiss_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap7b_swiss_organ PRIVATE
    Boost::mp11
    comdare::workload_driver)

# AP-7b-3/#27 -- SwissTable ISA group-match primitive, SIMD opt-in.
comdare_add_test(test_ap7b3_swiss_isa_group_match
    SOURCES test_ap7b3_swiss_isa_group_match.cpp)
target_include_directories(test_ap7b3_swiss_isa_group_match PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap7b3_swiss_isa_group_match PRIVATE
    Boost::mp11
    comdare::workload_driver)
# AP-8/#242 -- HdrHistogram_c wrapper + p95 host serializer coverage.
comdare_add_test(test_ap8_hdr_histogram
    SOURCES test_ap8_hdr_histogram.cpp)
target_include_directories(test_ap8_hdr_histogram PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_ap8_hdr_histogram PRIVATE
    comdare_hdr_histogram_wrapper
    comdare::workload_driver)

# P33-VAMPIR Option A — axis_06 VampirNfpAllocator conformance and statistics.
comdare_add_test(test_vampir_nfp_allocator
    SOURCES thesis_tiere/test_vampir_nfp_allocator.cpp)
comdare_prepare_thesis_object_dir(test_vampir_nfp_allocator)
target_include_directories(test_vampir_nfp_allocator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_vampir_nfp_allocator PRIVATE Boost::mp11)

# V41 #40 (Doku 24 §6.2) Tier->Organ-Rekonstruktions-Beleg (Tier ≡ Organ-Komposition ≡ std::map)
comdare_add_test(test_v41_axis_03a_tier_organ_equivalence
    SOURCES test_v41_axis_03a_tier_organ_equivalence.cpp)
target_include_directories(test_v41_axis_03a_tier_organ_equivalence PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_pruefling_merge PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.A AnatomyBase Wurzel + Gattungs-Marker (Mammal/Bird/Reptile/Invertebrate/Plant)
comdare_add_test(test_v41_anatomy_base
    SOURCES test_v41_anatomy_base.cpp)
target_include_directories(test_v41_anatomy_base PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_base PRIVATE Boost::mp11)

# V41.F.6.1.R5.C.A2 ExecutionEngine als Wurzel ueber AnatomyBase (Lebewesen vs Viren)
comdare_add_test(test_v41_execution_engine
    SOURCES test_v41_execution_engine.cpp)
target_include_directories(test_v41_execution_engine PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
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
    OUTPUT_DIR          "${PROJECT_BINARY_DIR}/generated/anatomy_modules"
    FINGERPRINT         "hot_pilot")

comdare_add_test(test_v41_anatomy_codegen
    SOURCES test_v41_anatomy_codegen.cpp)
target_include_directories(test_v41_anatomy_codegen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    OUTPUT_DIR          "${PROJECT_BINARY_DIR}/generated/anatomy_modules"
    FINGERPRINT         "wormhole_pilot")

comdare_add_test(test_v41_anatomy_module_loader
    SOURCES test_v41_anatomy_module_loader.cpp)
target_include_directories(test_v41_anatomy_module_loader PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_anatomy_module_loader PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
# Pfad zur Pilot-DLL via Compile-Define + Build-Dependency auf SHARED-Pilot
target_compile_definitions(test_v41_anatomy_module_loader PRIVATE
    "COMDARE_R5E_PILOT_DLL=\"$<TARGET_FILE:anatomy_codegen_pilot_wormhole_shared>\"")
add_dependencies(test_v41_anatomy_module_loader anatomy_codegen_pilot_wormhole_shared)

# V41.F.6.1.R5.D.3 Multi-Permutation-Codegen + load_all (3 Compositions als SHARED-DLLs)
set(_r5d3_pilot_dir "${PROJECT_BINARY_DIR}/generated/anatomy_modules_multi")
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware"
    "${PROJECT_BINARY_DIR}/generated/axes/simd"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_12_general_hardware PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.1.b axis_05 Memory-Layout (4 Wrappers, Goldstandard-konform)
comdare_add_test(test_v41_axis_05_memory_layout
    SOURCES test_v41_axis_05_memory_layout.cpp)
target_include_directories(test_v41_axis_05_memory_layout PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/layout"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_05_memory_layout PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.1.c+d axis_02 + axis_04 nodes (Goldstandard-Vollausbau in einem Test-File)
comdare_add_test(test_v41_axis_02_axis_04_nodes
    SOURCES test_v41_axis_02_axis_04_nodes.cpp)
target_include_directories(test_v41_axis_02_axis_04_nodes PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/path_compression"
    "${PROJECT_BINARY_DIR}/generated/axes/node"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_02_axis_04_nodes PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.a axis_07 Prefetch (4 Wrappers, Optional-Topics Start)
comdare_add_test(test_v41_axis_07_prefetch
    SOURCES test_v41_axis_07_prefetch.cpp)
target_include_directories(test_v41_axis_07_prefetch PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/prefetch_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_07_prefetch PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.b axis_11 Telemetry (4 Wrappers, Optional-Topics)
comdare_add_test(test_v41_axis_11_telemetry
    SOURCES test_v41_axis_11_telemetry.cpp)
target_include_directories(test_v41_axis_11_telemetry PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/telemetry_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})  # V41.F.2: axis_11_telemetry → axes/telemetry_axis
target_link_libraries(test_v41_axis_11_telemetry PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.c axis_10 Serialization (4 Wrappers, Optional-Topics)
comdare_add_test(test_v41_axis_10_serialization
    SOURCES test_v41_axis_10_serialization.cpp)
target_include_directories(test_v41_axis_10_serialization PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/serialization_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_10_serialization PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.d axis_14 ValueHandle (4 Wrappers, Optional-Topics)
comdare_add_test(test_v41_axis_14_value_handle
    SOURCES test_v41_axis_14_value_handle.cpp)
target_include_directories(test_v41_axis_14_value_handle PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/value_handle_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_14_value_handle PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.3 axis_08 Concurrency (8 Wrappers, Goldstandard)
comdare_add_test(test_v41_axis_08_concurrency
    SOURCES test_v41_axis_08_concurrency.cpp)
target_include_directories(test_v41_axis_08_concurrency PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/concurrency_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/topics/nodes/axis_01_page_type"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
        "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/filter_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_filter PRIVATE
    Boost::mp11)

# V41 #43 s4 — SuRF-Filter-Organ (axis_filter/composable): Filter-Property-Tests (no-FN + FP-Rate + Range)
comdare_add_test(test_v41_axis_filter_surf_organ
    SOURCES test_v41_axis_filter_surf_organ.cpp)
target_include_directories(test_v41_axis_filter_surf_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/filter_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_filter_surf_organ PRIVATE
    Boost::mp11)

# V41 SuRF S2 Inkrement 1 — echtes succinct LOUDS-Sparse-Filter-Organ (no-FN + FP-Tunbarkeit + S2>=S1-Kreuzbeleg)
comdare_add_test(test_v41_axis_filter_surf_louds_organ
    SOURCES test_v41_axis_filter_surf_louds_organ.cpp)
target_include_directories(test_v41_axis_filter_surf_louds_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/filter_axis"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_filter_surf_louds_organ PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.f axis_io (4 Wrappers + TYPED_TEST_SUITE Pattern)
comdare_add_test(test_v41_axis_io
    SOURCES test_v41_axis_io.cpp)
target_include_directories(test_v41_axis_io PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/io_dispatch"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})  # V41.F.2: axis_io → axes/io_dispatch migriert
target_link_libraries(test_v41_axis_io PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.g axis_migration (4 Wrappers + TYPED_TEST_SUITE Pattern)
comdare_add_test(test_v41_axis_migration
    SOURCES test_v41_axis_migration.cpp)
target_include_directories(test_v41_axis_migration PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/migration_policy"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})  # V41.F.2: axis_migration → axes/migration_policy
target_link_libraries(test_v41_axis_migration PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.h axis_01 IndexOrganization (4 Clustering-Wrappers + TYPED_TEST_SUITE)
comdare_add_test(test_v41_axis_01_index_organization
    SOURCES test_v41_axis_01_index_organization.cpp)
target_include_directories(test_v41_axis_01_index_organization PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/index_organization"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/simd"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_09_isa PRIVATE
    Boost::mp11)

# V41.F.6.1.R7.5.j axis_09b SIMD-Extension Sub-Achse (8 Wrappers + TYPED_TEST_SUITE)
comdare_add_test(test_v41_axis_09b_simd_extension
    SOURCES test_v41_axis_09b_simd_extension.cpp)
target_include_directories(test_v41_axis_09b_simd_extension PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/simd"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_axis_09b_simd_extension PRIVATE
    Boost::mp11)

# V41 R5.C.3 / #704 — Cross-Axis-Constraint ISA × SIMD-Extension (Permutationsraum-Korrektheit)
comdare_add_test(test_v41_r5c3_isa_simd_cross_constraint
    SOURCES test_v41_r5c3_isa_simd_cross_constraint.cpp)
target_include_directories(test_v41_r5c3_isa_simd_cross_constraint PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    "${PROJECT_BINARY_DIR}/generated/axes/simd"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
    "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_r5c3_isa_simd_cross_constraint PRIVATE
    Boost::mp11)

# axis_12 braucht jetzt auch axis_09b Include-Path
# V41.F.6.1.R5.I Tool zur Configure-Time aufrufen + Snippet inline include
# 2-Pass-Build: 1. cmake-Pass (Tool noch nicht gebaut, Warning) -> Tool bauen ->
# 2. cmake-Pass (Tool da, Snippet wird generiert + included)
include(anatomy_codegen_runner)
set(_r5i_snippet "${PROJECT_BINARY_DIR}/generated/r5i_perm_list.cmake")
set(_r5i_pilot_dir "${PROJECT_BINARY_DIR}/generated/r5i_anatomy_modules")
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
        "${PROJECT_BINARY_DIR}/generated")
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
    set(_pa_snippet   "${PROJECT_BINARY_DIR}/generated/comdare_perms_pa_list.cmake")
    set(_pa_pilot_dir "${PROJECT_BINARY_DIR}/generated/comdare_perms_pa_modules")
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
    set(_fj_snippet   "${PROJECT_BINARY_DIR}/generated/comdare_perms_full_join_list.cmake")
    set(_fj_pilot_dir "${PROJECT_BINARY_DIR}/generated/comdare_perms_full_join_modules")
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
set(_f15_snippet   "${PROJECT_BINARY_DIR}/generated/f15_perm_list.cmake")
set(_f15_pilot_dir "${PROJECT_BINARY_DIR}/generated/f15_measurement_modules")
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
        "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
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

# AP-9/#243: host-side provenance manifest sidecar, golden-/POD-neutral.
comdare_add_test(test_ap9_provenance_manifest
    SOURCES test_ap9_provenance_manifest.cpp)
target_include_directories(test_ap9_provenance_manifest PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap9_provenance_manifest PRIVATE
    comdare::workload_driver)

# AP-13/#247: host-side measurement-thread pinning, opt-in and ABI/golden neutral.
comdare_add_test(test_ap13_thread_pinning
    SOURCES test_ap13_thread_pinning.cpp)
target_include_directories(test_ap13_thread_pinning PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap13_thread_pinning PRIVATE
    comdare::workload_driver)
# AP-10/#244: Dataset-Akte manifest + Option-A string->uint64 corpus loader.
comdare_add_test(test_ap10_dataset_akte
    SOURCES test_ap10_dataset_akte.cpp)
target_include_directories(test_ap10_dataset_akte PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common/measurement/dataset_loader/include"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap10_dataset_akte PRIVATE
    comdare_dataset_loader
    comdare::workload_driver)

# #184 (2026-07-07): dataset_source -> Registry-loader_id -> Loader -> Operation HERMETISCH verdrahtet
# (committetes Klein-Fixture, KEIN on-demand-Fetch).
comdare_add_test(test_184_dataset_loader_wiring
    SOURCES test_184_dataset_loader_wiring.cpp)
target_include_directories(test_184_dataset_loader_wiring PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common/measurement/dataset_loader/include"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_184_dataset_loader_wiring PRIVATE
    comdare_dataset_loader
    comdare::workload_driver)
target_compile_definitions(test_184_dataset_loader_wiring PRIVATE
    "COMDARE_TEST_FIXTURE_DIR=\"${PROJECT_SOURCE_DIR}/tests/fixtures\"")

# #25 (2026-07-08): 6er-Kanon-Datensatz-Akten (url/protein/tpcds-id/trec-terms) gegen die ECHTEN
# im ce-Repo committeten CoCo-trie-preprocessed Dateien verifizieren (compute_dataset_akte reproduziert
# die in Code/test_data_xml/*.test_data.xml committeten checksum/line_count). CoCo-dataset = KEIN Submodul
# -> in CI vorhanden trotz GIT_SUBMODULE_STRATEGY=none.
comdare_add_test(test_25_kanon_dataset_akten
    SOURCES test_25_kanon_dataset_akten.cpp)
target_include_directories(test_25_kanon_dataset_akten PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common/measurement/dataset_loader/include"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_25_kanon_dataset_akten PRIVATE
    comdare_dataset_loader
    comdare::workload_driver)
target_compile_definitions(test_25_kanon_dataset_akten PRIVATE
    "COMDARE_CE_DATASET_DIR=\"${PROJECT_SOURCE_DIR}/ext/traversal/P04-CoCo-trie/CoCo-trie/dataset\"")

# #45 (2026-07-08): SOSD-Binaer-Loader (LE 8-Byte-Count + N*uint64 LE) -> Read-Ops; synthetische Blobs.
comdare_add_test(test_45_sosd_uint64_loader
    SOURCES test_45_sosd_uint64_loader.cpp)
target_include_directories(test_45_sosd_uint64_loader PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common/measurement/dataset_loader/include"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_45_sosd_uint64_loader PRIVATE
    comdare_dataset_loader
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

# #214 — GoF-Iterator-Organ: scan_range auf den vier Flach-Store-Traversal-Organen (header-only, std::map-Orakel).
comdare_add_test(test_v41_scan_range_organ
    SOURCES test_v41_scan_range_organ.cpp)
target_include_directories(test_v41_scan_range_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    # Hermetik (CI-2): measurement/measurable_concept.hpp liegt unter libs/cache_engine/src
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src")

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
    "${PROJECT_BINARY_DIR}/generated"
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_codegen_all_axes_umbrella PRIVATE Boost::mp11)

# V41.F.6.1 R5.G — End-to-End: auto-emittiertes AdHoc-Permutations-Modul → SHARED-DLL → laden + messen.
set(_r5g_adhoc_dir "${PROJECT_BINARY_DIR}/generated/r5g_adhoc_modules")
add_library(comdare_r5g_adhoc_perm SHARED auto_emitted_perm_module.cpp)
target_include_directories(comdare_r5g_adhoc_perm PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(comdare_r5g_adhoc_perm PRIVATE Boost::mp11)
target_compile_features(comdare_r5g_adhoc_perm PRIVATE cxx_std_23)
target_compile_definitions(comdare_r5g_adhoc_perm PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
set_target_properties(comdare_r5g_adhoc_perm PROPERTIES
    POSITION_INDEPENDENT_CODE                ON
    # #278: PREFIX "" — AnatomyModuleLoader::load_all filtert auf comdare_anatomy_perm_* (Dateiname),
    # CMakes lib-Praefix liess die DLL auf Linux durch den Filter fallen (handles==0).
    PREFIX                                   ""
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
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
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
set(_r5g_autobuilt_dir "${PROJECT_BINARY_DIR}/generated/r5g_autobuilt_modules")
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
            "${PROJECT_BINARY_DIR}/generated")
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
    test_d13_runtime_measure  # L-MEAS (HostMeasureLoop, ex RuntimeMeasureVisitor)
    test_d14_result_ingest    # L-CLUSTER gate-frei (result_ingest) + Audit-A1 MAJOR-MESS-09 Negativ-Tests (==176/binary_id)
    test_d14b_perm_runner     # L-CLUSTER-E2E gate-frei (perm_runner → result_ingest Round-Trip)
    test_a1_load_profile_opmix # Audit-A1 MAJOR-MESS-05 (op_mix-Pflicht: fehlend/leer → nullopt)
    test_d14c_e2e_pipeline    # L-CLUSTER-E2E gate-frei (durchgängige Pipeline, O(K))
    test_d15_selection_filter_chain # Doc 20 §C: CoR-Filterkette (S4 mess-getrieben, Auswertung->Generierung-Feedback)
    test_cmd2_container_attribution) # CMD-2/#252: host-seitige Container-in-SA-Attribution (c1 store_ops, ABI-neutral)
foreach(_dt IN LISTS COMDARE_GOALV6_LIGHT_DTESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common")
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    # #278 Klasse-B-Fix (2026-07-06, Job 213964): diese direkt-registrierten Standalone-Tests hingen NICHT
    # am comdare_tests-Sammel-Target -> CI baute sie nie -> 52x "Not Run". Property-Eintrag zieht sie in den
    # Sammel-Bau (inkl. automatischer Codegen-Hermetik-Kante aus dem Haupt-CMakeLists).
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_dt})
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
    test_axis_growth_policies                 # L-76b axis_growth Mehr-Policy-Achse (Doubling/GoldenRatio/FixedChunk/Exact + Registry)
    test_axis_view_policies)                  # L-76c View-Achsen extent/layout/accessor (mdspan-Policies + Registries)
foreach(_dt IN LISTS COMDARE_GOALV6_BOOST_DTESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_dt} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    # #278 Klasse-B-Fix (2026-07-06, Job 213964): diese direkt-registrierten Standalone-Tests hingen NICHT
    # am comdare_tests-Sammel-Target -> CI baute sie nie -> 52x "Not Run". Property-Eintrag zieht sie in den
    # Sammel-Bau (inkl. automatischer Codegen-Hermetik-Kante aus dem Haupt-CMakeLists).
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "goal_v6;phase_d")
endforeach()

# ─────────────────────────────────────────────────────────────────────────────
# #155 Phase-E-Standalone-Tests CTest-registrieren (vorher NICHT in CMake/ctest — nur als Quelle im Tree).
# Identisches Muster wie die GoalV6-D-Tests darueber: plain `int main()`-Standalone (KEIN gtest, daher add_test
# direkt, NICHT comdare_add_test), Return 0/1. KEIN DLL-/ADHOC-Bau — sie nutzen die normale cache-engine-Anatomie-
# Header (anatomy/ compositions/ axes/ builder/experiment_tree/), also den boost/generated-Include-Satz.
#   * test_migration_two_tier (#123, P4): 2-Tier IMigratableTier + organ_migrate_step + memento-parallel.
#   * test_seg_coverage       (P-MD3): seg_*_ns-Attributions-Coverage ueber perm_runner (ART/HOT/Masstree).
# test_v5_io_real_fixture ist leichter (nur std + Win32-API, kein Anatomie-Header) → eigener Block darunter.
set(COMDARE_PHASE_E_BOOST_TESTS
    test_migration_two_tier   # #123 P4 — 2-Tier-Migration (IMigratableTier + Memento parallel)
    test_seg_coverage         # P-MD3 — seg_*_ns-Attributions-Coverage (Mess-Pfad-Organzeit)
    # #155-Rest — 6 git-tracked aber bisher unregistrierte Phase-E-Verifikationstests (gleiche anatomy/axes-Kette).
    test_filter_real_from_keys        # Filter-Achse (Bloom/Cuckoo/Xor/RangeSURF) real aus echten Keys
    test_patricia_real                # path_compression-Achse (Patricia/None/RealTrie) real ersetzt
    test_value_handle_real            # value_handle-Achse (inline/external_pool/immutable/versioned/chain) real
    test_prefetch_real                # prefetch-Achse (RealDescent/HW/None/...) real ueber layout_aware_store
    test_prefetch_adversarial_verify  # prefetch-Achse adversarial-Verifikation
    test_prefetch_patha_t7            # prefetch Pfad-A T7 (measurable_workload)
    # #188-4c-iii Review-BLOCKER (2026-07-02): test_cow_memento war git-tracked aber NIE registriert — er ist
    # DIE Absicherung fuer tier_memento_is_copy_on_write()==true (CoW Rev. 2 + 4c-iii-Semantik-Delta: cow_capable_
    # haengt nur noch am echten container_t-Substrat). Gleiche anatomy/axes-Include-Kette wie die Tests darueber.
    test_cow_memento)                 # #133/#188-4c-iii — CoW-Flip-Gate (tier_memento_is_copy_on_write)
foreach(_dt IN LISTS COMDARE_PHASE_E_BOOST_TESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_dt} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    # #278 Klasse-B-Fix (2026-07-06, Job 213964): diese direkt-registrierten Standalone-Tests hingen NICHT
    # am comdare_tests-Sammel-Target -> CI baute sie nie -> 52x "Not Run". Property-Eintrag zieht sie in den
    # Sammel-Bau (inkl. automatischer Codegen-Hermetik-Kante aus dem Haupt-CMakeLists).
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "phase_e;standalone")
endforeach()

# ─────────────────────────────────────────────────────────────────────────────
# Goal-V4 G3 Batch-1 — Datenstruktur-Correctness-Pins (M-CE-01/03/04). Plain int main()-Standalone
# (KEIN gtest → add_test direkt), gleicher anatomy/axes-Include-Satz wie die Phase-E-Tests darueber.
#   * test_cuckoo_eviction_correctness — CuckooFilter-Verdraengung erhaelt Membership (keine False Negatives)
#   * test_hash_lookup_backward_shift  — HashLookup unregister == Knuth Algorithm R (keine False Misses/Duplikate)
#   * test_scalloc_realloc_alignment   — ScallocAllocator::reallocate spiegelt die alignment-Weiche
set(COMDARE_G3_CORRECTNESS_TESTS
    test_cuckoo_eviction_correctness
    test_hash_lookup_backward_shift
    test_scalloc_realloc_alignment)
foreach(_dt IN LISTS COMDARE_G3_CORRECTNESS_TESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_dt} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "g3;correctness;standalone")
endforeach()

# ─────────────────────────────────────────────────────────────────────────────
# Goal-V4 G4 Muster-F / M-CE-24 (2026-07-13, Test-Registrierungs-Audit) — 34 git-getrackte, bisher in KEINEM
# CMake-Target/ctest registrierte Standalone-Verifikationstests (plain int main(), g_fail-Zaehler → return 0/1;
# KEIN gtest → add_test DIREKT). Ihr dokumentierter Build-Weg (scratch_*.ps1) wurde bei der Behelfsweg-Bereinigung
# 2026-07-11 geloescht → ihre Pruefungen liefen NIE. Property-Eintrag zieht sie in den comdare_tests-Sammel-Bau
# (#278-Lektion). Gleiche anatomy/axes-Include-Kette + Boost::mp11 wie die #155-/GoalV6-Bloecke darueber.
#   * (B) COMDARE_MCE24_PLAIN_TESTS — ohne Mess-Defines.
#   * (A) COMDARE_MCE24_MEASURED_TESTS — mit COMDARE_MEASUREMENT_ON/STATISTICS (Observer-/Segment-Timer-Pfad;
#         ohne die Defines liefert der Observe-Pfad 0-Deltas → die Non-Stub-Assertions der Tests wuerden rot).
# ─────────────────────────────────────────────────────────────────────────────
set(COMDARE_MCE24_INCLUDES
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"                    # profile_to_tree.hpp -> xml_config_parser/xml_config_parser.hpp (Pfad-Form)
    "${PROJECT_SOURCE_DIR}/libs/common/serialization/xml_config_parser"  # test_kf1_thesis_profile_parser (bare xml_*.hpp)
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})

set(COMDARE_MCE24_PLAIN_TESTS
    test_br1_full22_count
    test_br1_subset
    test_br2_descriptor_foundation
    test_br2_roundtrip
    test_br3_obs22
    test_br_kf15_real
    test_clu_per_layout
    test_cow_capable_wrappers
    test_d4b_container_adapter
    test_genus_binding
    test_kf4_resource_control
    test_kf5_cacheline_weave
    test_kf6_node_run_body
    test_kf7_runtime_loop
    test_kf8_ceb_generator
    test_kf9_experiment_tree
    test_kf10_repetition_plan
    test_kf11_telemetry_silent
    test_kf12_kf13_slurm_prepare
    test_kf15_inverse_signature
    test_kf16_build_orchestrator
    test_kf16b_build_resilience)
foreach(_dt IN LISTS COMDARE_MCE24_PLAIN_TESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE ${COMDARE_MCE24_INCLUDES})
    target_link_libraries(${_dt} PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
    comdare_set_platform_defines(${_dt})
    add_test(NAME ${_dt} COMMAND ${_dt})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "g4;muster_f;standalone" TIMEOUT 120)
endforeach()

# HINWEIS (Muster-F-Befund, 2026-07-13 → RESOLVED gleichentags): der frühere STALE-Befund gegen das container-flip-
# Design (#188-4b) ist AUFGELÖST. Verdikt DESIGN-MANDATED-HONEST-0 per committetem #188-4c-i (Deep-Research
# 2026-07-13): die REFERENZ-Kompositionen (Art/Hot/Masstree/Wormhole) haben KEIN store_type → container_is_store_backed_
# = false → die store-slot-gescannten Observer (value_handle T11 / isa T12 / memory_layout-scan T5 / serialization-scan
# T9 / node_type T4 / höhere filled_axis_count / Node-Telemetrie) sind für die Hüllen EHRLICH 0 (Re-Kopplung = #234).
# Die per-op-gekoppelten Observer (search/telemetry) bleiben real >0. Die 4 Tests wurden auf diese honest-0-Erwartung
# repariert (filled_axis_count komposition-spezifisch: Art/Hot=10, Masstree=9; predicate_evals #24-neutralisiert) und
# sind jetzt regulär registriert:
#   test_obs_phaseA, test_obs_phaseB_pilot, test_obs_phaseB_t11_t12, test_d_v42_abi_telemetry_coupling.
set(COMDARE_MCE24_MEASURED_TESTS
    test_all19_segment_timer
    test_m8_storetrav_segment
    test_br3_observer
    test_obs_phaseB_t11_t12_iso
    test_obs_phaseB_t14_t16
    test_pathb_segment_timer
    test_segment_timer_differentiation
    test_obs_phaseA
    test_obs_phaseB_pilot
    test_obs_phaseB_t11_t12
    test_d_v42_abi_telemetry_coupling)
foreach(_dt IN LISTS COMDARE_MCE24_MEASURED_TESTS)
    add_executable(${_dt} "${_dt}.cpp")
    target_compile_features(${_dt} PRIVATE cxx_std_23)
    target_include_directories(${_dt} PRIVATE ${COMDARE_MCE24_INCLUDES})
    target_link_libraries(${_dt} PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
    comdare_set_platform_defines(${_dt})
    # Autoritative Observer-Rezeptur (wie test_harness_compile / thesis_adhoc / test_29 / test_ap15): ALLE DREI
    # Defines — ohne COMDARE_EXPERIMENT_MODE_ON bleiben der T11-value_handle-/T12-isa-store_observe-Pfad + Teile
    # der Node-Telemetrie 0 (Observe-Verdrahtung Phase 7). Der abi_adapter-Guard fordert MEASUREMENT (hier gesetzt).
    target_compile_definitions(${_dt} PRIVATE
        COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
    add_test(NAME ${_dt} COMMAND ${_dt})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_dt})
    set_tests_properties(${_dt} PROPERTIES LABELS "g4;muster_f;standalone;measured" TIMEOUT 120)
endforeach()

# test_kf1_thesis_profile_parser — separat, weil es das getrackte Studien-Profil parst und dessen Pfad als
# argv[1] erwartet (Default ist ein CWD-relativer ../../-Pfad → unter ctest-CWD nicht aufloesbar). Absoluter Pfad
# als add_test-Argument (CWD-unabhaengig; nur GELESEN). Include-/Link-Satz = COMDARE_MCE24_INCLUDES + XML-Parser-Lib.
add_executable(test_kf1_thesis_profile_parser "test_kf1_thesis_profile_parser.cpp")
target_compile_features(test_kf1_thesis_profile_parser PRIVATE cxx_std_23)
target_include_directories(test_kf1_thesis_profile_parser PRIVATE ${COMDARE_MCE24_INCLUDES})
target_link_libraries(test_kf1_thesis_profile_parser PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_kf1_thesis_profile_parser)
add_test(NAME test_kf1_thesis_profile_parser COMMAND test_kf1_thesis_profile_parser
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/cacheline_study.profile.xml")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_kf1_thesis_profile_parser)
set_tests_properties(test_kf1_thesis_profile_parser PROPERTIES LABELS "g4;muster_f;standalone" TIMEOUT 120)

# M-CE-09 (Muster-F, 2026-07-13) — schliesst die Test-Luecke der beiden als thread-safe deklarierten Lock-Free-
# Queues (LockFreeMPMCBuffer/Vyukov bounded MPMC + LockFreeSPSCBuffer): sie hatten repo-weit KEINEN einzigen
# nebenlaeufigen Test. Plain int main() (KEIN gtest) → add_test direkt; braucht std::thread (Threads::Threads).
# Der sanitize:tsan-Job kann dasselbe Target zusaetzlich unter -fsanitize=thread bauen (scharfe Race-Freiheit).
find_package(Threads REQUIRED)
add_executable(test_q1_lockfree_concurrency "test_q1_lockfree_concurrency.cpp")
target_compile_features(test_q1_lockfree_concurrency PRIVATE cxx_std_23)
target_include_directories(test_q1_lockfree_concurrency PRIVATE ${COMDARE_MCE24_INCLUDES})
target_link_libraries(test_q1_lockfree_concurrency PRIVATE Boost::mp11 Threads::Threads)
comdare_set_platform_defines(test_q1_lockfree_concurrency)
add_test(NAME test_q1_lockfree_concurrency COMMAND test_q1_lockfree_concurrency)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_q1_lockfree_concurrency)
set_tests_properties(test_q1_lockfree_concurrency PROPERTIES LABELS "g4;muster_f;standalone;queuing;concurrency" TIMEOUT 120)

# #155 — leichtes Phase-E-Fixture: nur std + Win32-API (CreateFileW/CreateFileMappingW/VirtualAlloc), KEIN
# Anatomie-/Boost-/generated-Include. Auf Non-Win32 kompiliert die #else-Branch zu einem sauberen SKIP (Exit 0).
add_executable(test_v5_io_real_fixture "test_v5_io_real_fixture.cpp")
target_compile_features(test_v5_io_real_fixture PRIVATE cxx_std_23)
comdare_set_platform_defines(test_v5_io_real_fixture)
if(WIN32)
    target_link_libraries(test_v5_io_real_fixture PRIVATE psapi)
endif()
add_test(NAME test_v5_io_real_fixture COMMAND test_v5_io_real_fixture)

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_v5_io_real_fixture)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_v5_io_real_fixture PROPERTIES LABELS "phase_e;standalone;io")

# #165-C (P-MD9, 2026-06-20) — Unit-Test fuer winsorized_mean_ns (latency_stats.hpp). Leichter Standalone-Test
# (plain int main, KEIN gtest → add_test direkt; KEIN Anatomie-/Boost-/generated-Include). Braucht nur den
# cache_engine-Include-Root, weil latency_stats.hpp via "builder/commands/latency_stats.hpp" inkludiert wird und
# selbst rein std-basiert ist (<algorithm>/<chrono>/<cstdint>/<span>/<vector>). Additiv neben den #155-Tests.
add_executable(test_winsorized_mean "test_winsorized_mean.cpp")
target_compile_features(test_winsorized_mean PRIVATE cxx_std_23)
target_include_directories(test_winsorized_mean PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
comdare_set_platform_defines(test_winsorized_mean)
add_test(NAME test_winsorized_mean COMMAND test_winsorized_mean)

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_winsorized_mean)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_winsorized_mean PROPERTIES LABELS "phase_e;standalone;stats")

# #223 (AUDIT-A3 K9, 2026-06-28) — Regressions-Test, der das Konformitaets-Gate (run_conformance_gate,
# builder/pruef_dock/conformance_gate.hpp) FESTNAGELT: eine konforme std::map-Huelle besteht, gezielt defekte
# Huellen (Lookup-Miss / falsches insert-NEU-Flag / size==0) werden GEFANGEN (Meta-Lehre #3 — nur konform-
# verifizierte Lebewesen liefern gueltige Mess-Daten). Standalone (plain int main, KEIN gtest/Boost/Anatomie-
# generated); zieht nur den cache_engine-Include-Root (conformance_gate.hpp -> <anatomy/idriveable_tier.hpp> +
# std). CI-erzwungen ueber die contract-Stage (contract:conformance, .gitlab-ci.yml) auf Linux-g++.
add_executable(test_conformance_gate "test_conformance_gate.cpp")
target_compile_features(test_conformance_gate PRIVATE cxx_std_23)
target_include_directories(test_conformance_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
comdare_set_platform_defines(test_conformance_gate)
add_test(NAME test_conformance_gate COMMAND test_conformance_gate)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_conformance_gate)
set_tests_properties(test_conformance_gate PROPERTIES LABELS "contract;standalone;gate")

# #188-4b-b0 (2026-07-01) — Weit-Key-Konformitaet der 9 Weg-B-Pool-Organe gegen std::map<u64,u64>. Prerequisite
# fuer 4b-b1 (container_-Umstellung Pool -> ObservableComposedContainer<organ>): belegt u64-Korrektheit +
# Truncation-Fix, den der truncation-blinde Prod-Gate (rng()%256) nie ausloest. GTest im Haupt-Suite — die 9
# Organe ziehen den vollen Stack (src/measurement + src/concepts + generierte path_compression-Flags), daher
# derselbe Include-Satz wie test_v41_topic_traversal (organ-komponierende Tests).
comdare_add_test(test_188_4bb0_pool_organ_wide_key_conformance
    SOURCES test_188_4bb0_pool_organ_wide_key_conformance.cpp)
target_include_directories(test_188_4bb0_pool_organ_wide_key_conformance PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_188_4bb0_pool_organ_wide_key_conformance PRIVATE Boost::mp11)

# #188-4b-b-V (PHASE V, 2026-07-01) — COMPILE-VERIFIKATION des 4b-b1b container_-Flips DURCH den ABI-ADAPTER.
# Explore+Codex-Befund: KEIN CI-Target instanziierte bisher SearchAlgorithmAbiAdapter<...> mit einer Pool-Familie
# (roher search_algo-Wrapper → pool_family_==true → container_ = ObservableComposedContainer<organ>); der Flip war
# nur Codex-/static_assert-verifiziert, NIE compiliert. Dieser Test instanziiert den Adapter fuer ALLE 9 Pool-
# Familien + treibt jeden ueber die Flip-Routing-Pfade (insert/lookup/erase/size/clear + tier_observe) → erzwingt die
# Template-Instanziierung des Pool-Zweigs = gruener Compile-Beleg. COMDARE_MEASUREMENT_ON=1 ZWINGEND (sonst kein
# IObservableTier → kein fill_observer_v3/fill_segment_timing_v3-Compile). Voller ADHOC-Include-Satz + Boost::mp11 wie
# m3v2_pmc_smoke. Standalone (plain int main, kein gtest) — CI-erzwungen ueber contract:pool_flip (.gitlab-ci.yml).
add_executable(test_188_4bbV_pool_adapter_flip_compile "test_188_4bbV_pool_adapter_flip_compile.cpp")
target_compile_features(test_188_4bbV_pool_adapter_flip_compile PRIVATE cxx_std_23)
target_include_directories(test_188_4bbV_pool_adapter_flip_compile PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_188_4bbV_pool_adapter_flip_compile PRIVATE Boost::mp11)
comdare_set_platform_defines(test_188_4bbV_pool_adapter_flip_compile)
target_compile_definitions(test_188_4bbV_pool_adapter_flip_compile PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_188_4bbV_pool_adapter_flip_compile COMMAND test_188_4bbV_pool_adapter_flip_compile)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_188_4bbV_pool_adapter_flip_compile)
set_tests_properties(test_188_4bbV_pool_adapter_flip_compile PROPERTIES LABELS "contract;standalone;pool_flip")

# S7-1 (#261/#234) -- BST-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_1_bst_pool_allocator_deg
    SOURCES test_s7_1_bst_pool_allocator_deg.cpp)
target_include_directories(test_s7_1_bst_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_1_bst_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_1_bst_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_1_bst_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# AP15-2 (#263-Rest, Strang C) -- get_allocator als POD-Proxy-Sub-Interface.
comdare_add_test(test_ap15_2_get_allocator_proxy
    SOURCES test_ap15_2_get_allocator_proxy.cpp)
target_include_directories(test_ap15_2_get_allocator_proxy PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap15_2_get_allocator_proxy PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_ap15_2_get_allocator_proxy PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_ap15_2_get_allocator_proxy PROPERTIES LABELS "contract;allocator")

# AP15-3 (#263-Rest, Strang C) -- HOST-CRTP-Map-Vertrag direkt gegen std::map, ohne Gate-Runner.
comdare_add_test(test_ap15_3_driveable_map_contract
    SOURCES test_ap15_3_driveable_map_contract.cpp)
target_include_directories(test_ap15_3_driveable_map_contract PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_ap15_3_driveable_map_contract PRIVATE comdare::workload_driver)
set_tests_properties(test_ap15_3_driveable_map_contract PROPERTIES LABELS "contract;gate")

# S7-2 (#261/#234) -- BTree-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_2_btree_pool_allocator_deg
    SOURCES test_s7_2_btree_pool_allocator_deg.cpp)
target_include_directories(test_s7_2_btree_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_2_btree_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_2_btree_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_2_btree_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# 234-V-a (2026-07-07, User-GO Option A): Shaped-Emission Mechanik + btree_order-Beweis --
# Trait-2-Arm + Adapter-ShapeCarrier + SHAPED-Render + organ_for!=void-Emitter-Filter; default-OFF,
# golden/ABI-4-neutral (Guards im Test). Spiegel des S7-2-Blocks (gleiche Deps/Defines).
comdare_add_test(test_234_va_shaped_adapter
    SOURCES test_234_va_shaped_adapter.cpp)
target_include_directories(test_234_va_shaped_adapter PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_234_va_shaped_adapter PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_234_va_shaped_adapter PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_234_va_shaped_adapter PROPERTIES LABELS "contract;pool_flip;shaped")

# #29 Schritt 1 (2026-07-08, AP-15): comdare::container Kopf-Framework -- additive golden/ABI-neutrale
# Generalisierung der 4 gebauten Container-Genus als generische Typen (kein Anatomie-/ABI-Kern-Umbau).
comdare_add_test(test_29_container_framework
    SOURCES test_29_container_framework.cpp)
target_include_directories(test_29_container_framework PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_29_container_framework PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_29_container_framework PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# Goal-V3 Striktheit (Metaprogrammierung): prospektiver compile-time-Guard. Beweist POSITIV, dass die 11
# Referenz-Anatomien nicht-polymorph (keine vtable im Hot-Path) + AnatomyConcept-erfuellt sind, und dass die
# ABI-Grenze (IAnatomyBase/IDriveableTier/IMeasurableWorkload) bewusst polymorph bleibt -- faengt jede
# compile-time->runtime-Degradation. Additiv, golden/ABI-neutral (reiner static_assert-Test ueber Bestandstypen).
comdare_add_test(test_striktheit_metaprog_guard
    SOURCES test_striktheit_metaprog_guard.cpp)
target_include_directories(test_striktheit_metaprog_guard PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_striktheit_metaprog_guard PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_striktheit_metaprog_guard PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# Teil 2 (#50, Organ-Ebene): alle 26 Achsen-Strategie-Marker nicht-polymorph. Separate Datei, weil
# registry_to_axis_levels.hpp die 23 topic_config_sets zieht (schwerer Header, isoliert retriggerbar).
comdare_add_test(test_striktheit_axes_guard
    SOURCES test_striktheit_axes_guard.cpp)
target_include_directories(test_striktheit_axes_guard PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_striktheit_axes_guard PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_striktheit_axes_guard PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# #50-Rest (ABI-Transport-POD-Striktheit): zentraler Guard, dass alle 12 Snapshot-PODs trivially_copyable
# + standard_layout bleiben (C-ABI-sicher über die anatomy_module_abi_v1-DLL-Grenze). Additiv, ABI-neutral.
comdare_add_test(test_striktheit_abi_pod_guard
    SOURCES test_striktheit_abi_pod_guard.cpp)
target_include_directories(test_striktheit_abi_pod_guard PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_striktheit_abi_pod_guard PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_striktheit_abi_pod_guard PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# Teil 4 (GO-3 A2, Task #5 Hebel-A-Rest, 2026-07-12): Mess-Kern-Reinheits-Guard — Interface-Freeze der
# 9 *_scan-Kernel-Familien (bewusst skalare, strided Mess-Kerne: Zugriffsmuster == Achsen-Signal, Doc 21 §F;
# prospektiv build-brechend bei kuenftiger Isa-Einspeisung). Bewusst LEICHTE Header (nur Achsen-Registries +
# axis_09_isa_amd64.hpp, KEINE config_sets — Cold-Cache-ICE-Lektion, s. test_striktheit_axes_guard).
comdare_add_test(test_striktheit_scan_kernel_purity
    SOURCES test_striktheit_scan_kernel_purity.cpp)
target_include_directories(test_striktheit_scan_kernel_purity PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_striktheit_scan_kernel_purity PRIVATE Boost::mp11)
set_tests_properties(test_striktheit_scan_kernel_purity PROPERTIES LABELS "striktheit;axes")
set_tests_properties(test_29_container_framework PROPERTIES LABELS "contract;gattung;container")

# 234-V-b (2026-07-08): Rest-Familien Shaped-Naht + binary_id-Shape-Segment + Baum->Emitter-Verdrahtung.
# Spiegel des 234-V-a-Blocks (gleiche Deps/Defines), default-OFF und golden-neutral.
comdare_add_test(test_234_vb_rest_families_shaped
    SOURCES test_234_vb_rest_families_shaped.cpp)
target_include_directories(test_234_vb_rest_families_shaped PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_234_vb_rest_families_shaped PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_234_vb_rest_families_shaped PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_234_vb_rest_families_shaped PROPERTIES LABELS "contract;pool_flip;shaped")

# CMD-1-a (#267/#251) -- Compile-time-Command-Basis + Mess-Visitor-Slot.
comdare_add_test(test_cmd1_a_axis_command_base
    SOURCES test_cmd1_a_axis_command_base.cpp)
target_include_directories(test_cmd1_a_axis_command_base PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_cmd1_a_axis_command_base PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_cmd1_a_axis_command_base PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_cmd1_a_axis_command_base PROPERTIES LABELS "contract")

# S7-3 (#261/#234) -- SkipList-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_3_skip_list_pool_allocator_deg
    SOURCES test_s7_3_skip_list_pool_allocator_deg.cpp)
target_include_directories(test_s7_3_skip_list_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_3_skip_list_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_3_skip_list_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_3_skip_list_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-4 (#261/#234) -- Hash-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_4_hash_pool_allocator_deg
    SOURCES test_s7_4_hash_pool_allocator_deg.cpp)
target_include_directories(test_s7_4_hash_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_4_hash_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_4_hash_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_4_hash_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-5 (#261/#234) -- ART-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_5_art_pool_allocator_deg
    SOURCES test_s7_5_art_pool_allocator_deg.cpp)
target_include_directories(test_s7_5_art_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_5_art_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_5_art_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_5_art_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-6 (#261/#234) -- HOT-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_6_hot_pool_allocator_deg
    SOURCES test_s7_6_hot_pool_allocator_deg.cpp)
target_include_directories(test_s7_6_hot_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_6_hot_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_6_hot_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_6_hot_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-7 (#261/#234) -- START-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_7_start_pool_allocator_deg
    SOURCES test_s7_7_start_pool_allocator_deg.cpp)
target_include_directories(test_s7_7_start_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_7_start_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_7_start_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_7_start_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-8 (#261/#234) -- Wormhole-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_8_wormhole_pool_allocator_deg
    SOURCES test_s7_8_wormhole_pool_allocator_deg.cpp)
target_include_directories(test_s7_8_wormhole_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_8_wormhole_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_8_wormhole_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_8_wormhole_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-9 (#261/#234) -- SuRF-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_9_surf_pool_allocator_deg
    SOURCES test_s7_9_surf_pool_allocator_deg.cpp)
target_include_directories(test_s7_9_surf_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_9_surf_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_9_surf_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_9_surf_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# S7-10 (#261/#234) -- SwissTable-Pool: allocator-param default + T6 allocator DEG hook.
comdare_add_test(test_s7_10_swiss_pool_allocator_deg
    SOURCES test_s7_10_swiss_pool_allocator_deg.cpp)
target_include_directories(test_s7_10_swiss_pool_allocator_deg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s7_10_swiss_pool_allocator_deg PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_s7_10_swiss_pool_allocator_deg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_s7_10_swiss_pool_allocator_deg PROPERTIES LABELS "contract;pool_flip;allocator")

# #234-F1 (W2, 2026-07-02) — BtreeOrder-Shape-Varianten real: Kt2/3/8/16-Organe paargenau vs std::map + Struktur-
# Wirkungsanker (Kt2-vs-Kt16-Knotenzahl bei gleicher 200-Key-Sequenz) + Level-0-Alias-Anker. Test-only-Increment
# (das BTree-Traversal-Organ liest Pool::kT/kMaxKeys bereits ueberall). GTest im Haupt-Suite — zieht die
# Composed-Organe + topics-Shape-Strategien, daher der 4bb0-Include-Satz. CI: contract:node_shape.
comdare_add_test(test_234_f1_btree_order_shape
    SOURCES test_234_f1_btree_order_shape.cpp)
target_include_directories(test_234_f1_btree_order_shape PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_234_f1_btree_order_shape PRIVATE Boost::mp11)

# #234-F2 (W2, 2026-07-02) — SkipListShape-Varianten real: draw_level zieht P=kPNumerator/kPDenominator
# maskenbasiert (Level-0 P=1/2 DRAW-FUER-DRAW bit-treu zur alten Muenzwurf-Formel — der Test beweist es ueber
# 12000 Ziehungen gegen die lokal nachgebaute Alt-Referenz, gleicher Seed 0xC0FFEE); P25-Statistik-Anker
# (100k Draws, 13-15sigma-Toleranzen); Max8/Max32-Grenzen; Shaped-Organe paargenau vs std::map.
comdare_add_test(test_234_f2_skip_list_shape
    SOURCES test_234_f2_skip_list_shape.cpp)
target_include_directories(test_234_f2_skip_list_shape PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_234_f2_skip_list_shape PRIVATE Boost::mp11)

# #234-F3 (W2, 2026-07-02) — BstShape-Varianten real: U32/U16-Index-Packing paargenau vs std::map + U16-Ueberlauf-
# Beweis (fuellt exakt Store::kNilIndex Knoten, dann length_error; Free-List-Recycling an der Kapazitaetsgrenze) +
# kNil-/kIndexBytes-static_asserts + Level-0-Alias-Anker.
comdare_add_test(test_234_f3_bst_shape
    SOURCES test_234_f3_bst_shape.cpp)
target_include_directories(test_234_f3_bst_shape PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_234_f3_bst_shape PRIVATE Boost::mp11)

# #234-F4 (W2, 2026-07-02) — HashProbeShape real: Shape-LF-Trigger (Lf50/70/90, Lf70 termgleich zum Alt-Code) +
# Chaining-Vollausbau (conditional_t-Datenpfade OA/Chain im Store, Concept-Union Core+OA+Chaining,
# for_each_record-Ketten-Walk): Konformitaet je Variante + LF-Wirkungs-Beweis (bucket_count 128 vs 64 nach N=64)
# + Chaining-Recycling/tombstones==0 + Level-0-Alias-Anker.
comdare_add_test(test_234_f4_hash_probe_shape
    SOURCES test_234_f4_hash_probe_shape.cpp)
target_include_directories(test_234_f4_hash_probe_shape PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_234_f4_hash_probe_shape PRIVATE Boost::mp11)

# #188-4a (2026-07-02) — Eytzinger als 10. organ_for-Familie (Option b): EytzingerLayoutStore (sortierter
# Primaerzustand + lazy BFS-Puffer in EINEM Store) + EytzingerTraversalOrgan (branch-free Kern bit-identisch
# zum Wrapper) + ComposedEytzingerSearch (for_each_record = DEG-1-Key-Ernte-Fix). T1 std::map-Konformitaet,
# T2 Wrapper-Aequivalenz (u16-Keyraum), T3 Option-b-dirty-Semantik, T4 Ernte ohne Rebuild-Trigger,
# T5 branch-free-Grenzfaelle, T6 Kopier-Memento, T7 Observable-Huelle, T8 static_asserts (organ_for/traversal_for).
comdare_add_test(test_188_4a_eytzinger_organ
    SOURCES test_188_4a_eytzinger_organ.cpp)
target_include_directories(test_188_4a_eytzinger_organ PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_188_4a_eytzinger_organ PRIVATE Boost::mp11)

# #188-4c-0 (2026-07-02) — Known-Compositions-Conformance-Gate: alle 11 Reference-/PaperBinding-Compositions
# durch run_conformance_gate (std::map-Orakel) am realen SearchAlgorithmAbiAdapter — Sicherheitsnetz fuer den
# Spiegel-Abbau (#188-4c). Routing-Snapshot seit #188-4c-i: routes_through_store==true (Huellen authoritativ).
comdare_add_test(test_188_4c0_known_compositions_conformance
    SOURCES test_188_4c0_known_compositions_conformance.cpp)
target_include_directories(test_188_4c0_known_compositions_conformance PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_188_4c0_known_compositions_conformance PRIVATE Boost::mp11)

# #216-H2 — Load-Phase darf die Run-Statistik nicht kontaminieren: tier_reset_statistics() nullt nach Load
# daten-erhaltend die auto-gekoppelten Achsen-Statistiken am realen SearchAlgorithmAbiAdapter.
comdare_add_test(test_216h2_stat_reset_after_load
    SOURCES test_216h2_stat_reset_after_load.cpp)
target_include_directories(test_216h2_stat_reset_after_load PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_216h2_stat_reset_after_load PRIVATE Boost::mp11)
target_compile_definitions(test_216h2_stat_reset_after_load PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# G3 Batch-2 — additive Correctness-Guards am realen SearchAlgorithmAbiAdapter: (a) uint16-Narrow-Cast-Guard
# im Mapping-Observer-Seitenpfad (out-of-range-Keys ehrlich abgelehnt statt still gewrappt), (b) nullptr-Guard-
# Happy-Path-Neutralitaet ueber alle drei run_workload-Treiber. Messungs-/Statistik-Build wie test_216h2.
comdare_add_test(test_abi_adapter_g3_batch2
    SOURCES test_abi_adapter_g3_batch2.cpp)
target_include_directories(test_abi_adapter_g3_batch2 PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_abi_adapter_g3_batch2 PRIVATE Boost::mp11)
target_compile_definitions(test_abi_adapter_g3_batch2 PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# #221 RC prefetch_distance — Laufzeit-RC erreicht den realen T7 Store-Prefetch (tier_observe-Pfad B).
comdare_add_test(test_221_rc_prefetch_distance
    SOURCES test_221_rc_prefetch_distance.cpp)
target_include_directories(test_221_rc_prefetch_distance PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_221_rc_prefetch_distance PRIVATE Boost::mp11)
target_compile_definitions(test_221_rc_prefetch_distance PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
target_compile_options(test_221_rc_prefetch_distance PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/UCOMDARE_A06_IS_ORIGINAL_CODEGEN>"
    "$<$<CXX_COMPILER_ID:MSVC>:/UCOMDARE_A03A_IS_ORIGINAL_CODEGEN>"
    "$<$<CXX_COMPILER_ID:MSVC>:/UCOMDARE_AQ1_IS_ORIGINAL_CODEGEN>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UCOMDARE_A06_IS_ORIGINAL_CODEGEN>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UCOMDARE_A03A_IS_ORIGINAL_CODEGEN>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UCOMDARE_AQ1_IS_ORIGINAL_CODEGEN>")

# Schicht E1 (#221 re-impl): T6/T8/T1/T11-RC-Konsum + A-Fix-Edges, bewusst kein contract-Label.
comdare_add_test(test_e1_contract_rc_konsum
    SOURCES test_e1_contract_rc_konsum.cpp)
target_include_directories(test_e1_contract_rc_konsum PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_e1_contract_rc_konsum PRIVATE Boost::mp11)
target_compile_definitions(test_e1_contract_rc_konsum PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
target_compile_options(test_e1_contract_rc_konsum PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/UCOMDARE_A06_IS_ORIGINAL_CODEGEN>"
    "$<$<CXX_COMPILER_ID:MSVC>:/UCOMDARE_A03A_IS_ORIGINAL_CODEGEN>"
    "$<$<CXX_COMPILER_ID:MSVC>:/UCOMDARE_AQ1_IS_ORIGINAL_CODEGEN>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UCOMDARE_A06_IS_ORIGINAL_CODEGEN>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UCOMDARE_A03A_IS_ORIGINAL_CODEGEN>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UCOMDARE_AQ1_IS_ORIGINAL_CODEGEN>")
set_tests_properties(test_e1_contract_rc_konsum PROPERTIES LABELS "e1;rc")

# #188-4c-ii (2026-07-02) — Flach-Wrapper-Traversal-Gate: die 4 letzten markerlosen Flach-Wrapper (Array256/
# Array65535/VectorU8U8/VectorU16U16) sind store-traversierbar (DirectAddress-/SortedVectorTraversal) —
# Conformance-Oracle + routes_through_store==true + Wide-Key-/Sparse-Korrektur-Proben. Damit ist der
# search_organ_+Spiegel-Zweig fuer ALLE Kompositionen tot (4c-iii entfernt ihn).
comdare_add_test(test_188_4cii_flat_wrapper_traversal
    SOURCES test_188_4cii_flat_wrapper_traversal.cpp)
target_include_directories(test_188_4cii_flat_wrapper_traversal PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_188_4cii_flat_wrapper_traversal PRIVATE Boost::mp11)

# #217-2a (2026-07-03) — optionale Container-Kapazitaet: nur Static-Container (Array256/Array65535)
# lehnen tier_insert-Keys ausserhalb der deklarierten Kapazitaet ab; Advisory-Container bleiben unbounded.
comdare_add_test(test_217_2a_capacity_guard
    SOURCES test_217_2a_capacity_guard.cpp)
target_include_directories(test_217_2a_capacity_guard PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_217_2a_capacity_guard PRIVATE Boost::mp11)
target_compile_definitions(test_217_2a_capacity_guard PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# P5 (#156-De-Risk, 2026-06-20) — Smoke fuer die PMC-WIDE-Naht: make_pmc_source()/IPmcSource im LAZY WIDE-Mess-Pfad
# (perm_runner) + 7 additive pmc_*-Spalten in lazy_csv_header. Standalone (plain int main, baut KEINE DLLs), zieht
# aber den schweren iterator-/pmc_source_factory-Kontext -> derselbe anatomy/axes/Boost::mp11-Include-Satz wie
# COMDARE_PHASE_E_BOOST_TESTS + die Mess-Mode-Defines (COMDARE_MEASUREMENT_ON/STATISTICS/EXPERIMENT_MODE_ON), unter
# denen perm_runner die PMC-Naht aktiviert. OFFIZIELLE CMake/ctest-Registrierung (ersetzt das fruehere Scratch-
# Build-Skript). NullPmcSource (COMDARE_ENABLE_PMC OFF) -> alle 7 Spalten 0/available=0 (ehrliche Null).
add_executable(m3v2_pmc_smoke "thesis_tiere/m3v2_pmc_smoke.cpp")
comdare_prepare_thesis_object_dir(m3v2_pmc_smoke)
target_compile_features(m3v2_pmc_smoke PRIVATE cxx_std_23)
target_include_directories(m3v2_pmc_smoke PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(m3v2_pmc_smoke PRIVATE Boost::mp11)
comdare_set_platform_defines(m3v2_pmc_smoke)
target_compile_definitions(m3v2_pmc_smoke PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME m3v2_pmc_smoke COMMAND m3v2_pmc_smoke)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS m3v2_pmc_smoke)
set_tests_properties(m3v2_pmc_smoke PROPERTIES LABELS "phase_e;standalone;pmc")

# #230-Rest (W1, 2026-07-02) - Contract-Compile-Gate fuer den Host-Treiber-Stack des thesis_tiere-Harness:
# cache_engine_builder_iterator.hpp + perm_runner.hpp + side-effect-freie Baum-/CSV-/Runner-Datenpfade. Kein
# run_lazy, keine DLLs, keine Messung, keine CSV-Dateien - nur Compile+Smoke der Treiber-Schicht vor den
# Node-Shape-Achsen. Gleicher schwerer Include-/Define-Satz wie m3v2_pmc_smoke.
add_executable(test_harness_compile "test_harness_compile.cpp")
target_compile_features(test_harness_compile PRIVATE cxx_std_23)
target_include_directories(test_harness_compile PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_harness_compile PRIVATE Boost::mp11)
comdare_set_platform_defines(test_harness_compile)
target_compile_definitions(test_harness_compile PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_harness_compile COMMAND test_harness_compile)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_harness_compile)
set_tests_properties(test_harness_compile PROPERTIES LABELS "contract;standalone;harness")

# Goal-V4 G4 / M-CE-05 (2026-07-13) - dedizierter Unit-Test der BINARY-Resume-Logik
# lazy_try_resume_binary (#139 Mess-Resume-Schutz) aus cache_engine_builder_iterator.hpp. Deckt die drei
# Kern-Pfade: (a) valider per-Binary-Stand -> Resume/Skip, (b) korrupt/partiell/leer -> sauberer Fallback
# (false, kein Crash/UB), (c) veraltet (Version-/Schema-/Format-Mismatch) -> Neustart. Temp-Dateien
# user-eindeutig (comdare_test_tmp, #278/#24). Gleicher schwerer Include-/Define-Satz wie test_harness_compile
# (derselbe Host-Treiber-Header). Ergaenzt das PROFIL-Pfad-Gate (test_profile_roundtrip) um die BINARY-Ebene.
add_executable(test_lazy_resume_binary "test_lazy_resume_binary.cpp")
target_compile_features(test_lazy_resume_binary PRIVATE cxx_std_23)
target_include_directories(test_lazy_resume_binary PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_lazy_resume_binary PRIVATE Boost::mp11)
comdare_set_platform_defines(test_lazy_resume_binary)
target_compile_definitions(test_lazy_resume_binary PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_lazy_resume_binary COMMAND test_lazy_resume_binary)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_lazy_resume_binary)
set_tests_properties(test_lazy_resume_binary PROPERTIES LABELS "contract;standalone;harness;resume")
# AP-6/#240 rev: rein lesendes Profil-Coverage-Gate (SOTA P01-P33 + Allokator A01-A23).
comdare_add_test(test_profile_coverage
    SOURCES thesis_tiere/test_profile_coverage.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_profile_coverage)
target_compile_definitions(test_profile_coverage PRIVATE
    "COMDARE_ALGORITHM_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles\"")
set_tests_properties(test_profile_coverage PROPERTIES LABELS "contract;profiles;coverage")

# AP-2-neu/#236 (W4, 2026-07-07): Katalog-Pfad ist der EINZIGE PRT-ART-Mess-Pfad — materialisiert die
# ECHTE PrtArtComposition STUB-FREI (Gegenstueck zu den QUARANTAENE-Notizen der 3 Alt-Pfade im prt-art-Repo).
comdare_add_test(test_ap2_katalog_pfad_stubfrei
    SOURCES thesis_tiere/test_ap2_katalog_pfad_stubfrei.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_ap2_katalog_pfad_stubfrei)
target_include_directories(test_ap2_katalog_pfad_stubfrei PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ap2_katalog_pfad_stubfrei PRIVATE Boost::mp11)
target_compile_definitions(test_ap2_katalog_pfad_stubfrei PRIVATE
    "COMDARE_CE_LIB_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine\"")

# #18 Golden-Coverage-Gate: jede der 19 Achsen ist im Coverage-Test beruehrbar — seit #26/GO-5 tragen ALLE
# 19 Achsen (inkl. der 4 Basis-Achsen) einen dedizierten Sweep-Katalog. Echter Konsument der
# SweepCatalog-Instanzen (source_catalog.hpp).
comdare_add_test(test_axis_sweep_coverage
    SOURCES thesis_tiere/test_axis_sweep_coverage.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_axis_sweep_coverage)
target_include_directories(test_axis_sweep_coverage PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_axis_sweep_coverage PRIVATE Boost::mp11)

# #26/GO-5 (2026-07-12): Gate fuer das Smoke-/Coverage-MESS-Profil m3_smoke_coverage.profile.xml + den
# Multi-Sweep-Durchlauf (profile_sweep_passes). Pinnt die Binary-Zaehlung (Default-Configure dieses Baums:
# 72 Sweep-/Baseline-DLLs + 21 SOTA = 93; USE=ENABLE&&HAVE-abhaengig, s. Test-Kopf), beweist golden-
# Baseline-Identitaet (golden[0] byte-unberuehrt) und validiert das Profil gegen die realen
# EnabledStrategies (ctest-Gegenstueck zu `messung_driver --validate`).
comdare_add_test(test_smoke_coverage_profile
    SOURCES thesis_tiere/test_smoke_coverage_profile.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_smoke_coverage_profile)
target_include_directories(test_smoke_coverage_profile PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_smoke_coverage_profile PRIVATE Boost::mp11)
target_compile_definitions(test_smoke_coverage_profile PRIVATE
    "COMDARE_THESIS_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles\""
    "COMDARE_GOLDEN_320_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\"")
set_tests_properties(test_smoke_coverage_profile PROPERTIES LABELS "contract;profiles;coverage;smoke")

# GO-5 Forks 1/2/6 (2026-07-12) — ctest-Gate fuer die ADDITIVEN E4-Profil-Schema-Erweiterungen
# <datasets> (Fork 1; Single-Source = die test_data-Akten, Fork 2/R2) + <sota_series fairness=..>
# (Fork 6, Thesis §sec:fairness). Beweist Parse-Roundtrip (wdk_fairness_example.profile.xml),
# Default-Neutralitaet (m3v2/golden NUR GELESEN; Report bestehender Profile byte-identisch),
# validate-Fehlerfaelle und die Konsum-Ankunft in der Runner-Spec (SotaPass.fairness_mode) +
# im Export (CSV-Endspalte fairness_mode + resume-v5-Stamp mit fair=/datasets=-Segmenten).
comdare_add_test(test_wdk_datasets_fairness
    SOURCES thesis_tiere/test_wdk_datasets_fairness.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_wdk_datasets_fairness)
target_include_directories(test_wdk_datasets_fairness PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_wdk_datasets_fairness PRIVATE Boost::mp11)
target_compile_definitions(test_wdk_datasets_fairness PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_THESIS_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles\""
    "COMDARE_GOLDEN_320_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\"")
set_tests_properties(test_wdk_datasets_fairness PROPERTIES LABELS "contract;profiles;e4;go5")

# INC-3 Familie A (2026-07-14) — ctest-Gate der ADDITIVEN E4-Profil-Schema-Erweiterung
# <measurement_categories>/<category name=..> (die dritte W/D/K-Dimension K; Verortungs-Hinweis K nun GEBAUT).
# Beweist (a) gueltige Kategorien geparst+validiert (wdk_fairness_example.profile.xml), (b) ungueltige Kategorie
# ⇒ Fehler (Single-Source kMeasurementAxisRegistry), (c) ohne Element ⇒ leer+ok+Report byte-identisch
# (rueckwaerts-kompatibel), (d) Duplikat ⇒ WARNUNG, (e) golden-320-Baseline-binary_id unveraendert (Projektion,
# keine Achse). m3v2_study.profile.xml + golden_fullpilot_320_binary_ids.txt werden NUR GELESEN (TABU-Wache).
comdare_add_test(test_measurement_categories
    SOURCES thesis_tiere/test_measurement_categories.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_measurement_categories)
target_include_directories(test_measurement_categories PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_measurement_categories PRIVATE Boost::mp11)
target_compile_definitions(test_measurement_categories PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_THESIS_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles\""
    "COMDARE_GOLDEN_320_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\"")
set_tests_properties(test_measurement_categories PROPERTIES LABELS "contract;profiles;e4;inc3")

# INC-D (2026-07-14) — ctest-Gate des comdare_experiment-Parsers als MODUL im allgemeinen ce-Parser
# (parse_experiment_profile, common-Schicht) + der layering-sauberen validate_experiment_profile
# (cache_engine-Schicht). Beweist (a) Golden-Instanz parst woertlich, (b) validiert ok gegen die REALE
# ce-Registry + prt-Registry, (c) 3 Fehlerfaelle (ungueltige merge / allowed_variant nicht-in-Registry /
# != 2 engines). Fixtures (experiment_golden.xml + prt_art_axis_registry.xml) werden NUR GELESEN; die
# reale cache_engine_axis_registry.xml ist Single-Source (per Define referenziert, nicht kopiert).
comdare_add_test(test_experiment_parser
    SOURCES thesis_tiere/test_experiment_parser.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_experiment_parser)
target_include_directories(test_experiment_parser PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_experiment_parser PRIVATE Boost::mp11)
target_compile_definitions(test_experiment_parser PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_EXPERIMENT_GOLDEN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/experiment_golden.xml\""
    "COMDARE_CE_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml\""
    "COMDARE_PRT_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/prt_art_axis_registry.xml\"")
set_tests_properties(test_experiment_parser PROPERTIES LABELS "contract;profiles;e4;incd")

# M-CE-10 (Voll-Review 2026-07-13) — ctest-Gate der Stufe2_PrueflingReplace-Korrektur im sota_catalog:
# St2 ist KONZEPTIONELL EINE Binary (HOT-Pilot). Beweist (1) Dedup mehrerer St2-Deklarationen zu 1 Pass,
# (2) host-dominante H2-Attribution (h2_lebewesen == "hot", NICHT das angefragte lebewesen — via committeter
# Akte), (3) distinkte binary_id-Zaehlung am gepinnten m3v2_study (14 statt vorher 20/21 gezaehlt),
# (4) fairness-Varianten werden NICHT ueber die gemeinsame binary_id zusammengefasst. m3v2_study.profile.xml +
# sota_h2_scores.xml werden NUR GELESEN (TABU-Wache). comdare_add_test registriert auto in COMDARE_TEST_TARGETS.
comdare_add_test(test_sota_st2_dedup
    SOURCES thesis_tiere/test_sota_st2_dedup.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_sota_st2_dedup)
target_include_directories(test_sota_st2_dedup PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_sota_st2_dedup PRIVATE Boost::mp11)
target_compile_definitions(test_sota_st2_dedup PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_THESIS_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles\""
    "COMDARE_SOTA_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/sota\"")
set_tests_properties(test_sota_st2_dedup PROPERTIES LABELS "contract;profiles;e4;anti-phantom")

# M-CE-12 (Voll-Review 2026-07-13) — ctest-Gate der rein-lesenden <workloads>-id-Pruefung in
# validate_profile: eine getippte <workloads>-id (z.B. das alte "A" statt "ycsb_a") wird von --validate
# ABGELEHNT (statt erst im teuren E4-Lauf mit exit 4). Beweist (a) UNBEKANNTE id rejected, (b) ycsb_*
# akzeptiert, (c) leere Auswahl OK, (d) 2-arg-Aufruf rueckwaerts-kompatibel (Pruefung uebersprungen),
# (e) Regressions-Wache M-CE-11 (m3v2_smoke/m3v2_sota_pilot gegen die REAL entdeckten load_profiles/-ids).
# SCHEMA.md:73-Verweis (test_validate_profile.cpp) damit wieder wahr. comdare_add_test registriert das
# Target auto in COMDARE_TEST_TARGETS (comdare_tests-Sammel-Bau; #278-Lektion).
comdare_add_test(test_validate_profile
    SOURCES thesis_tiere/test_validate_profile.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_validate_profile)
target_include_directories(test_validate_profile PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_validate_profile PRIVATE Boost::mp11)
target_compile_definitions(test_validate_profile PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_THESIS_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles\"")
set_tests_properties(test_validate_profile PROPERTIES LABELS "contract;profiles;e4;validate")

# GO-5 Fork 7 (2026-07-12, Dossier 20260712-go5 A.7) — ctest-Gate fuer die TOOL-BERECHNETE
# H2-Code-Qualitaets-Akte (sota_h2_scores.xml, NEUE Schwester-Datei der byte-unberuehrten
# sota/*.profile.xml) + die additive CSV-Endspalte h2_code_quality_score. Beweist Akte-Parse
# + 1:1-Vollabdeckung je sota-Profil, tool-berechnete Scores der vendorten Quellen, honest-n/a
# (Rekonstruktionen/prt_art/fehlende Akte), Formel-Single-Source (h2_score_akte.hpp) und
# CSV-/Resume-Stamp-Invarianz. Der Generator (apps/h2_score_akte_tool) laeuft hier NICHT
# (cppcheck-gebunden) — die committete Akte ist die Fixture.
comdare_add_test(test_h2_score_akte
    SOURCES thesis_tiere/test_h2_score_akte.cpp)
comdare_prepare_thesis_object_dir(test_h2_score_akte)
target_include_directories(test_h2_score_akte PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_h2_score_akte PRIVATE Boost::mp11)
target_compile_definitions(test_h2_score_akte PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_SOTA_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/sota\"")
set_tests_properties(test_h2_score_akte PROPERTIES LABELS "contract;profiles;e4;go5")

# CE-DL2 (Task #152) — LINUX-PMC-Smoke fuer die LinuxPerfPmcSource (perf_event_open). Spiegel von m3v2_pmc_smoke,
# aber LINUX-CONDITIONAL registriert: nur unter (UNIX AND NOT APPLE) — der Windows-/MSVC-Build sieht dieses Target
# NIE (kein add_executable, keine Linux-Header). make_pmc_source() liefert auf Linux+COMDARE_ENABLE_PMC eine
# LinuxPerfPmcSource; der Smoke klammert begin()/end() um eine speicherruehrende Pointer-Chasing-Schleife und
# prueft LITERAL: Zugriff -> available==1 UND >=1 Counter != 0; kein Zugriff (paranoid/EACCES/Container) ->
# sauberer Skip (Exit 0, available==0), KEIN Crash, KEINE erfundenen Werte. counter!=0 ist HW-/Policy-abhaengig
# (prod-Runner) und wird NICHT auf jedem Runner garantiert. Standalone (plain int main, baut KEINE DLLs).
if(UNIX AND NOT APPLE)
    add_executable(linux_perf_pmc_smoke "thesis_tiere/linux_perf_pmc_smoke.cpp")
    comdare_prepare_thesis_object_dir(linux_perf_pmc_smoke)
    target_compile_features(linux_perf_pmc_smoke PRIVATE cxx_std_23)
    target_include_directories(linux_perf_pmc_smoke PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    comdare_set_platform_defines(linux_perf_pmc_smoke)
    target_compile_definitions(linux_perf_pmc_smoke PRIVATE
        COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
    add_test(NAME linux_perf_pmc_smoke COMMAND linux_perf_pmc_smoke)
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS linux_perf_pmc_smoke)
    set_tests_properties(linux_perf_pmc_smoke PROPERTIES LABELS "phase_e;standalone;pmc;linux")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Goal-V6 L-76/L-74a — DLL-Round-Trips als OFFIZIELLE CMake-Targets (vorher nur committete PS1-Skripte). Die 5
# boost-freien Permutations-/Build-Varianten-DLLs (interne Organe) + ihre Loader-Tests. Die DLL-Pfade gehen via
# $<TARGET_FILE:..> als add_test-Argumente → kein Lade-Ort-Rätselraten (Multi-Config-robust). C++23.
# ─────────────────────────────────────────────────────────────────────────────
set(_goalv6_dll_dir "${PROJECT_BINARY_DIR}/generated/goalv6_dll_modules")
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

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_dgenus_dll)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_dgenus_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

# Build-Varianten-DLL-Loader-Test (L-74a, win32 LoadLibrary/GetProcAddress — Avx512 vs Avx2 über die .dll-Grenze).
add_executable(test_buildvariant_dll test_buildvariant_dll.cpp)
target_include_directories(test_buildvariant_dll PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine" "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_features(test_buildvariant_dll PRIVATE cxx_std_23)
add_dependencies(test_buildvariant_dll perm_buildvariant_avx512 perm_buildvariant_avx2)
add_test(NAME test_buildvariant_dll COMMAND test_buildvariant_dll
    "$<TARGET_FILE:perm_buildvariant_avx512>" "$<TARGET_FILE:perm_buildvariant_avx2>")

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_buildvariant_dll)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_buildvariant_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

# L-74a — Build-Varianten-DLLs mit den ECHTEN axis_01/09b/12-Wrapper-KLASSEN (DenseBytePageType/Avx512SimdExtension/
# X86_64HardwareProfile) statt real-geformter Stubs. Brauchen die generierten flags.hpp-Dirs + Boost::mp11 → eigene
# Registrierung (nicht in der boost-freien Schleife oben). Beweist: die REALEN Library-Wrapper sind DLL-einbaubar.
#
# GO-3 A1 (Task #5 Hebel-A-Rest, 2026-07-12) — DEKLARATIONS-WAHRHEIT: die real-Wrapper-DLLs bauen jetzt MIT
# den echten ISA-Flags (comdare_apply_simd_extension_flags AVX512/AVX2, cmake/isa_features.cmake) und
# emittieren das CHECKED-Makro (consteval-Kohaerenz-Guard: deklarierte Extension == Build-ISA-Stufe). Vorher
# deklarierten sie Avx512/Avx2 OHNE Flags — Mess-Etikett != Mess-Gegenstand (Dossier GO3 §2.3).
#  * BUILD-Gate: nur wenn der Compiler BEIDE Flags kennt (sonst wuerde der CHECKED-Guard zu Recht feuern).
#  * AUSFUEHRUNGS-Gate: nur wenn der BUILD-Host die ISA real kann (__builtin_cpu_supports-Probe, Muster der
#    Dispatch-Test-Varianten unten) — die DLLs enthalten jetzt echten AVX2-/AVX-512-Maschinencode; schon der
#    Inspect-Symbol-Prolog darf auf schwaecheren Hosts nicht SIGILLen.
include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    check_cxx_compiler_flag("/arch:AVX2" COMDARE_CXX_HAS_ARCH_AVX2)
    check_cxx_compiler_flag("/arch:AVX512" COMDARE_CXX_HAS_ARCH_AVX512)
    set(COMDARE_CXX_HAS_SIMD_EXT_AVX2_FLAG "${COMDARE_CXX_HAS_ARCH_AVX2}")
    set(COMDARE_CXX_HAS_SIMD_EXT_AVX512_FLAG "${COMDARE_CXX_HAS_ARCH_AVX512}")
else()
    check_cxx_compiler_flag("-mavx2" COMDARE_CXX_HAS_MAVX2)
    check_cxx_compiler_flag("-mavx512f" COMDARE_CXX_HAS_MAVX512F)
    set(COMDARE_CXX_HAS_SIMD_EXT_AVX2_FLAG "${COMDARE_CXX_HAS_MAVX2}")
    set(COMDARE_CXX_HAS_SIMD_EXT_AVX512_FLAG "${COMDARE_CXX_HAS_MAVX512F}")
endif()
check_cxx_source_runs("int main(){return __builtin_cpu_supports(\"avx2\")?0:1;}" COMDARE_HOST_RUNS_AVX2)
check_cxx_source_runs("int main(){return __builtin_cpu_supports(\"avx512f\")?0:1;}" COMDARE_HOST_RUNS_AVX512F)

if(COMDARE_CXX_HAS_SIMD_EXT_AVX2_FLAG AND COMDARE_CXX_HAS_SIMD_EXT_AVX512_FLAG)
    set(_rsrc_perm_buildvariant_real_avx512 genus_buildvariant_real_avx512.cpp)
    set(_rsrc_perm_buildvariant_real_avx2   genus_buildvariant_real_avx2.cpp)
    foreach(_rdll IN ITEMS perm_buildvariant_real_avx512 perm_buildvariant_real_avx2)
        add_library(${_rdll} SHARED "${_rsrc_${_rdll}}")
        target_include_directories(${_rdll} PRIVATE
            "${PROJECT_SOURCE_DIR}/libs/cache_engine"
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
            "${PROJECT_SOURCE_DIR}/libs/common"
            "${PROJECT_BINARY_DIR}/generated"
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
    # GO-3 A1: die je-Extension-Flags aus der ZENTRALEN Kaskade (keine Dreifach-Spiegelung von -m-Flags hier).
    comdare_apply_simd_extension_flags(perm_buildvariant_real_avx512 AVX512)
    comdare_apply_simd_extension_flags(perm_buildvariant_real_avx2 AVX2)
    add_dependencies(test_buildvariant_dll perm_buildvariant_real_avx512 perm_buildvariant_real_avx2)

    if(COMDARE_HOST_RUNS_AVX2 AND COMDARE_HOST_RUNS_AVX512F)
        add_test(NAME test_buildvariant_dll_real COMMAND test_buildvariant_dll
            "$<TARGET_FILE:perm_buildvariant_real_avx512>" "$<TARGET_FILE:perm_buildvariant_real_avx2>")

        set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_buildvariant_dll_real)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
        set_tests_properties(test_buildvariant_dll_real PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

        # GO-3 A1 Kohaerenz-Beweis-Test: Etikett (POD simd_width_bits) == Realitaet (Build-ISA-Kaskade via
        # comdare_build_isa_width_probe) je real-DLL, literal ueber die echte .dll-Grenze gezogen.
        add_executable(test_ap5_simd_extension_coherence test_ap5_simd_extension_coherence.cpp)
        target_include_directories(test_ap5_simd_extension_coherence PRIVATE
            "${PROJECT_SOURCE_DIR}/libs/cache_engine" "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
        target_compile_features(test_ap5_simd_extension_coherence PRIVATE cxx_std_23)
        add_dependencies(test_ap5_simd_extension_coherence
            perm_buildvariant_real_avx512 perm_buildvariant_real_avx2)
        add_test(NAME test_ap5_simd_extension_coherence COMMAND test_ap5_simd_extension_coherence
            "$<TARGET_FILE:perm_buildvariant_real_avx512>" "$<TARGET_FILE:perm_buildvariant_real_avx2>")
        set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_ap5_simd_extension_coherence)
        set_tests_properties(test_ap5_simd_extension_coherence PROPERTIES LABELS "goal_v6;axes;simd")
    else()
        message(STATUS "GO-3 A1: test_buildvariant_dll_real/test_ap5_simd_extension_coherence NICHT registriert "
            "(Host ohne AVX2+AVX-512F — real-DLLs werden nur GEBAUT, Muster Dispatch-Varianten)")
    endif()
else()
    message(STATUS "GO-3 A1: real-Wrapper-DLLs NICHT gebaut (Compiler ohne AVX2-/AVX-512-Flag — CHECKED-Guard "
        "wuerde zu Recht feuern)")
endif()

# GO-3 A1 NEGATIV-BEWEIS (konfigurationszeitig, Muster isa_features.cmake; Dossier-GO3-Risiko 3 — kein
# flake-anfaelliger ctest-Fall): der consteval-Kohaerenz-Guard MUSS eine inkohaerente Deklaration ABLEHNEN.
# Probe-Paar: eine 256-bit-Deklaration (echte Wrapper-API-Form) OHNE AVX2-Flag DARF NICHT kompilieren
# (static_assert im Guard), MIT AVX2-Flag MUSS sie kompilieren (die Paarung faengt falsche Negativa wie
# kaputte Include-Pfade ab). Bei Guard-Erosion: harter Configure-FATAL.
if(COMDARE_ARCH_X86_64 AND COMDARE_CXX_HAS_SIMD_EXT_AVX2_FLAG)
    include(CheckCXXSourceCompiles)
    set(_ap5_coherence_probe_src "
        #include <topics/hardware/axis_09b_simd_extension/axis_09b_build_coherence.hpp>
        #include <string_view>
        struct ProbeAvx2Decl {
            static constexpr bool is_active() noexcept { return true; }
            static constexpr int vector_width_bits() noexcept { return 256; }
            static constexpr bool compatible_with_x86() noexcept { return true; }
            static constexpr bool compatible_with_arm() noexcept { return false; }
            static constexpr bool compatible_with_riscv() noexcept { return false; }
            static constexpr std::string_view flag_suffix() noexcept { return \"AVX2\"; }
        };
        static_assert(::comdare::cache_engine::hardware::axis_09b_simd_extension::
                          declared_extension_matches_build<ProbeAvx2Decl>());
        int main() { return 0; }")
    set(CMAKE_REQUIRED_INCLUDES "${PROJECT_SOURCE_DIR}/libs/cache_engine")
    set(CMAKE_REQUIRED_FLAGS "")
    check_cxx_source_compiles("${_ap5_coherence_probe_src}" COMDARE_AP5_COHERENCE_NEGATIVE_PROBE_COMPILES)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_REQUIRED_FLAGS "/arch:AVX2")
    else()
        set(CMAKE_REQUIRED_FLAGS "-mavx2")
    endif()
    check_cxx_source_compiles("${_ap5_coherence_probe_src}" COMDARE_AP5_COHERENCE_POSITIVE_PROBE_COMPILES)
    set(CMAKE_REQUIRED_FLAGS "")
    set(CMAKE_REQUIRED_INCLUDES "")
    if(COMDARE_AP5_COHERENCE_NEGATIVE_PROBE_COMPILES)
        message(FATAL_ERROR "GO-3 A1: Kohaerenz-Guard WIRKUNGSLOS — 256-bit-Deklaration kompiliert OHNE "
            "AVX2-Flag (der static_assert in axis_09b_build_coherence.hpp haette feuern muessen).")
    endif()
    if(NOT COMDARE_AP5_COHERENCE_POSITIVE_PROBE_COMPILES)
        message(FATAL_ERROR "GO-3 A1: Kohaerenz-Positiv-Probe kompiliert NICHT mit AVX2-Flag — Guard/Probe "
            "defekt (ein falsches Negativ bliebe sonst unerkannt).")
    endif()
    message(STATUS "GO-3 A1 Kohaerenz-Guard bewiesen: Negativ-Probe (ohne AVX2-Flag) korrekt ABGELEHNT, "
        "Positiv-Probe (mit AVX2-Flag) kompiliert")
endif()

# L-74a — ADHOC-BUILDVARIANT-DLL: EINE DLL traegt die 17-Slot-Anatomie (4 ABI-Symbole) UND die 3-Build-Achsen-
# Inspection (comdare_build_variant_inspect). Heavy (Anatomie-Umbrella + Boost + generierte Dirs, wie comdare_r5g).
add_library(perm_adhoc_buildvariant SHARED genus_adhoc_buildvariant.cpp)
target_include_directories(perm_adhoc_buildvariant PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
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

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_adhoc_buildvariant_dll)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_adhoc_buildvariant_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

# L-MEAS — HostMeasureLoop (ex RuntimeMeasureVisitor) über die ECHTE geladene DLL (dynamic_cast IObservableTier + IResourceControllableTier
# aus DEMSELBEN Tier → Host-Bridge → Mess-Schleife je dyn. Einstellung × repeats, kein Reload). Reuse perm_adhoc_buildvariant.
add_executable(test_d13_dll_runtime_measure test_d13_dll_runtime_measure.cpp)
target_include_directories(test_d13_dll_runtime_measure PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine" "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_features(test_d13_dll_runtime_measure PRIVATE cxx_std_23)
target_link_libraries(test_d13_dll_runtime_measure PRIVATE comdare::anatomy_module_loader)
add_dependencies(test_d13_dll_runtime_measure perm_adhoc_buildvariant)
add_test(NAME test_d13_dll_runtime_measure COMMAND test_d13_dll_runtime_measure "$<TARGET_FILE:perm_adhoc_buildvariant>")

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_d13_dll_runtime_measure)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_d13_dll_runtime_measure PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip")

message(STATUS "Goal-V6 Phase D: ${CMAKE_CURRENT_LIST_LINE} — 28 add_test-D-Tests (16 leicht + 12 Boost) + 5 DLL-Round-Trip-Tests "
    "(test_dgenus_dll/test_buildvariant_dll{,_real}/test_adhoc_buildvariant_dll/test_d13_dll_runtime_measure) als OFFIZIELLE CMake-Targets (6 SHARED-DLLs via $<TARGET_FILE>). "
    "container_dll (test_d4b) bleibt scratch-verifiziert (Folge).")

# ─────────────────────────────────────────────────────────────────────────────
# #172/#174/#175 LEICHTE Werkzeug-Tests CTest-registrieren (vorher NUR via ad-hoc build_*.bat/.ps1 baubar).
# Selbe Begründung + selbes Muster wie der #155-Block + die GoalV6-D-Tests: plain `int main()`-Standalone
# (KEIN gtest → add_test DIREKT, NICHT comdare_add_test), Return 0/1. KEIN DLL-/ADHOC-Bau, KEIN Boost/mp11
# und KEIN rekursiver generated/-Include nötig — die zugehörigen Werkzeug-Header haben einen schlanken,
# stabilen Include-Satz (verifiziert: nur std + libs/cache_engine{,/builder/...} + libs/common).
#
#   * test_cacheline_policy_selector (#174, Versprechen 2): CacheLinePolicySelector (GoF Strategy). Inkludiert
#       anatomy/resource_controllable_tier.hpp + builder/cacheline_policy/cacheline_policy_selector.hpp
#       (zieht builder/algorithm_resource_control.hpp) → Include-Root libs/cache_engine. Liegt direkt unter
#       tests/unit/. Optionales CSV-Argument: der Test SKIPt sauber (Exit 0), wenn die untracked Pilot-CSV
#       fehlt — der Heuristik-Kern-Beleg ist davon unabhängig, daher reproduzierbar ohne Fixture.
#   * test_load_profile_writer (#175, Versprechen 3): write_load_profile_xml + extract_load_profile_from_
#       measurements. Quelle liegt in libs/cache_engine/builder/workload_driver/. Inkludiert load_profile_
#       parser.hpp (→ <serialization/.../xml_reader.hpp> aus libs/common, header-only/inline) + load_profile_
#       writer.hpp. BRAUCHT Fixtures: ein LP-XML-Verzeichnis (getracktes algorithm_profiles/load_profiles)
#       + eine Mess-CSV. Die echte Pilot-CSV ist untracked (build/) → für die G5-Reproduzierbarkeit wird
#       eine schlanke, GETRACKTE Fixture-CSV (tests/unit/fixtures/load_profile_writer_pilot.csv) als Argument
#       verdrahtet; Output landet im Build-Tree. So baut + läuft der Test aus einem frischen git-clone grün.

# Output-Wurzel für die Test-Schreibvorgänge: bevorzugt das OS-Temp-Verzeichnis (TMPDIR/TEMP/TMP),
# sonst der Build-Tree. Grund: wenn der Build-Tree unter einem synchronisierenden Ordner (OneDrive)
# liegt, blockiert der Datei-Sync jeden geschriebenen Roundtrip-/Extract-XML minutenlang (verifiziert:
# 478 s im OneDrive-Build-Tree vs 1 s im OS-Temp). Das ist umgebungs-, nicht code-bedingt; der Test
# bleibt korrekt, aber wir halten ihn schnell + suite-tauglich, indem wir ins Temp schreiben.
set(_comdare_test_out_root "${CMAKE_CURRENT_BINARY_DIR}")
foreach(_tmpvar TMPDIR TEMP TMP)
    if(DEFINED ENV{${_tmpvar}} AND IS_DIRECTORY "$ENV{${_tmpvar}}")
        set(_comdare_test_out_root "$ENV{${_tmpvar}}")
        break()
    endif()
endforeach()

# (a) test_cacheline_policy_selector — Quelle unter tests/unit/, std-only Include-Satz.
add_executable(test_cacheline_policy_selector "test_cacheline_policy_selector.cpp")
target_compile_features(test_cacheline_policy_selector PRIVATE cxx_std_23)
target_include_directories(test_cacheline_policy_selector PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")
comdare_set_platform_defines(test_cacheline_policy_selector)
# Optionales CSV-Arg (echte Pilot-CSV, falls vorhanden) — fehlt sie, SKIPt der Test diesen Teil sauber.
add_test(NAME test_cacheline_policy_selector COMMAND test_cacheline_policy_selector
    "${PROJECT_SOURCE_DIR}/build/thesis_tiere/m3v2_sota_pilot_measurements.csv")

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_cacheline_policy_selector)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
set_tests_properties(test_cacheline_policy_selector PROPERTIES
    LABELS "tools;cacheline;strategy" TIMEOUT 60)

# (b) test_load_profile_writer — Quelle in libs/.../workload_driver/, header-only Chain + getrackte Fixtures.
add_executable(test_load_profile_writer
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/workload_driver/test_load_profile_writer.cpp")
target_compile_features(test_load_profile_writer PRIVATE cxx_std_23)
target_include_directories(test_load_profile_writer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/workload_driver"  # load_profile_parser/_writer.hpp
    "${PROJECT_SOURCE_DIR}/libs/common")                                # serialization/.../xml_reader.hpp
comdare_set_platform_defines(test_load_profile_writer)
add_test(NAME test_load_profile_writer COMMAND test_load_profile_writer
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/load_profiles"   # arg1: getracktes LP-XML-Verzeichnis
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/load_profile_writer_pilot.csv"        # arg2: getrackte Fixture-Mess-CSV
    "${_comdare_test_out_root}/comdare_load_profile_writer_out")                # arg3: Output (Temp, nicht OneDrive)

set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_load_profile_writer)  # #278 Not-Run-(b): in comdare_tests-Sammel-Bau (Job 214367)
# TIMEOUT großzügig: 1 s im OS-Temp, aber Sync-/AV-Umgebungen können Datei-Writes verzögern.
set_tests_properties(test_load_profile_writer PROPERTIES
    LABELS "tools;workload_driver;xml_export" TIMEOUT 300)

message(STATUS "tools-tests: test_cacheline_policy_selector (#174) + test_load_profile_writer (#175) "
    "als add_test registriert (best_binary_selector-Tool = tools/best_binary_selector add_executable).")

# ─────────────────────────────────────────────────────────────────────────────
# GO4/#8 F-C (2026-07-12) — C1/C2-Unterachsen-Tests CTest-registrieren (Muster #155-/#174-Bloecke:
# plain `int main()`-Standalone, KEIN gtest -> add_test DIREKT, Return 0/1).
#   * test_kf3_cacheline_subaxis (C1): pinnt den cacheline-Wertraum (jetzt 60 = 4x3x5; B32 additiv,
#       KF-5 thesis-treu). War bisher NUR Quelle im Tree (nie in ctest) -> der Produkt-Pin war zahnlos;
#       ab jetzt laeuft er in der Suite. Schlanker Include-Satz (nur axes/cacheline/ -> Root libs/cache_engine).
#   * test_ff2_node_width_subaxis (C2): die NEUE FF2-Unterachse "Knoten-Breite in Cache-Lines" (1..16) —
#       Wertraum + binary-id-Neutralitaet ohne Profil-Aktivierung (profile_to_tree) + REALER Konsum im
#       Node-Layout-Pfad (LayoutAwareChunkedStore-Chunk-Geometrie). Include-Satz wie die #155-Phase-E-Tests
#       (+ libs/common/serialization) + XML-Parser-Lib fuer den Parse des getrackten Studien-Profils (arg1).
add_executable(test_kf3_cacheline_subaxis "test_kf3_cacheline_subaxis.cpp")
target_compile_features(test_kf3_cacheline_subaxis PRIVATE cxx_std_23)
target_include_directories(test_kf3_cacheline_subaxis PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")
comdare_set_platform_defines(test_kf3_cacheline_subaxis)
add_test(NAME test_kf3_cacheline_subaxis COMMAND test_kf3_cacheline_subaxis)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_kf3_cacheline_subaxis)
set_tests_properties(test_kf3_cacheline_subaxis PROPERTIES LABELS "axes;cacheline;kf3" TIMEOUT 60)

add_executable(test_ff2_node_width_subaxis "test_ff2_node_width_subaxis.cpp")
target_compile_features(test_ff2_node_width_subaxis PRIVATE cxx_std_23)
target_include_directories(test_ff2_node_width_subaxis PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_ff2_node_width_subaxis PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_ff2_node_width_subaxis)
add_test(NAME test_ff2_node_width_subaxis COMMAND test_ff2_node_width_subaxis
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/ff2_node_width_study.profile.xml")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_ff2_node_width_subaxis)
set_tests_properties(test_ff2_node_width_subaxis PROPERTIES LABELS "axes;cacheline;ff2;node_width" TIMEOUT 120)

# ─────────────────────────────────────────────────────────────────────────────
# GO4/#8 F-B (2026-07-12) — alloc_hw-Unterachsen-Test CTest-registrieren (Muster test_ff2-Block:
# plain `int main()`-Standalone, KEIN gtest -> add_test DIREKT, Return 0/1).
#   * test_fb_alloc_hw_subaxis (F-B): die NEUE Unterachse "NUMA/Page->allocator" (alloc_hw.numa_node
#       {auto,0,1} + alloc_hw.page {4k,2m}) — Wertraum + HW-Gate (axis_12-Properties als Typ-Parameter,
#       gate_alloc_hw_for<HW>) + binary-id-Neutralitaet ohne Profil-Aktivierung (profile_to_tree) +
#       REALER Konsum an der Allocator-Kante (NUMAlloc-Node-Bindung statt Hartcode -1; Pool-Page-Hint ->
#       pmr::pool_options). Include-/Link-Satz identisch zum ff2-Block; Studien-Profil als arg1.
add_executable(test_fb_alloc_hw_subaxis "test_fb_alloc_hw_subaxis.cpp")
target_compile_features(test_fb_alloc_hw_subaxis PRIVATE cxx_std_23)
target_include_directories(test_fb_alloc_hw_subaxis PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_fb_alloc_hw_subaxis PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_fb_alloc_hw_subaxis)
add_test(NAME test_fb_alloc_hw_subaxis COMMAND test_fb_alloc_hw_subaxis
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/fb_numa_page_study.profile.xml")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_fb_alloc_hw_subaxis)
set_tests_properties(test_fb_alloc_hw_subaxis PROPERTIES LABELS "axes;alloc;fb;alloc_hw" TIMEOUT 120)

# 2026-07-11: best_binary_selector parse+rank Characterization-Test. Das #172-Werkzeug war bisher NUR als
# add_executable (tools/best_binary_selector) registriert, NIE als add_test -> parse_measurement_csv +
# rank_binaries (ausgelieferte Produktionslogik) waren UNGETESTET. Dieser Test nagelt sie gate-frei fest
# (synthetische WIDE-CSV-Golden-Fixture, kein #156-Messlauf, kein Treiber). best_binary_selector.cpp ist
# self-contained (nur Standardbibliothek) -> direkt in die Test-TU kompiliert, keine Engine-/Boost-Deps.
add_executable(test_best_binary_selector_parse_rank
    "test_best_binary_selector_parse_rank.cpp"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector/best_binary_selector.cpp")
target_compile_features(test_best_binary_selector_parse_rank PRIVATE cxx_std_23)
target_include_directories(test_best_binary_selector_parse_rank PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector")
comdare_set_platform_defines(test_best_binary_selector_parse_rank)
# (REV-DATA-04, WP-5 2026-07-16): Fixture 2 = strikte Zahl-Validierung (nan/inf/12junk/leer-Pflicht verworfen).
add_test(NAME test_best_binary_selector_parse_rank COMMAND test_best_binary_selector_parse_rank
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/best_binary_wide.csv"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/best_binary_strict_values.csv")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_best_binary_selector_parse_rank)
set_tests_properties(test_best_binary_selector_parse_rank PROPERTIES LABELS "tools;best_binary" TIMEOUT 60)

# Schicht E4 Paket B - XML-Eingang der Resource-Control-Felder bis zur E3-AxisLevel-Struktur.
add_executable(test_e4_contract_xml_to_axislevels "test_e4_contract_xml_to_axislevels.cpp")
target_compile_features(test_e4_contract_xml_to_axislevels PRIVATE cxx_std_23)
target_include_directories(test_e4_contract_xml_to_axislevels PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization")
target_link_libraries(test_e4_contract_xml_to_axislevels PRIVATE comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_e4_contract_xml_to_axislevels)
add_test(NAME test_e4_contract_xml_to_axislevels COMMAND test_e4_contract_xml_to_axislevels
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/e4_contract_profile.xml")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_e4_contract_xml_to_axislevels)
# Label BEWUSST ohne 'contract': ce test:unit laeuft mit -LE 'contract|pmc', und die dedizierten
# contract-Gates matchen nur explizite -R-Listen -- mit 'contract'-Label waere der Test CI-tot.
set_tests_properties(test_e4_contract_xml_to_axislevels PROPERTIES LABELS "profiles;e4")

# Schicht E3 Paket A - #223-Gate-Beweis im echten perm_runner-Pfad.
add_executable(test_e3_contract_conformance_gate_wirksam "test_e3_contract_conformance_gate_wirksam.cpp")
target_compile_features(test_e3_contract_conformance_gate_wirksam PRIVATE cxx_std_23)
target_include_directories(test_e3_contract_conformance_gate_wirksam PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")
# run_workload_perm (Review-Fix wf_6e518da1) zieht workload_driver-Symbole (profile_by_name etc.).
target_link_libraries(test_e3_contract_conformance_gate_wirksam PRIVATE comdare::workload_driver)
comdare_set_platform_defines(test_e3_contract_conformance_gate_wirksam)
add_test(NAME test_e3_contract_conformance_gate_wirksam COMMAND test_e3_contract_conformance_gate_wirksam)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_e3_contract_conformance_gate_wirksam)
set_tests_properties(test_e3_contract_conformance_gate_wirksam PROPERTIES LABELS "e3;gate")

# Schicht E3 Paket B - StaticBinaryView-Bijektion und dynamische Levels ohne DLL-Bau.
add_executable(test_e3_contract_binary_id_bijektion "test_e3_contract_binary_id_bijektion.cpp")
target_compile_features(test_e3_contract_binary_id_bijektion PRIVATE cxx_std_23)
target_include_directories(test_e3_contract_binary_id_bijektion PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")
comdare_set_platform_defines(test_e3_contract_binary_id_bijektion)
add_test(NAME test_e3_contract_binary_id_bijektion COMMAND test_e3_contract_binary_id_bijektion)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_e3_contract_binary_id_bijektion)
set_tests_properties(test_e3_contract_binary_id_bijektion PROPERTIES LABELS "e3;tree")

# Schicht E2 Paket B - ABI-Vertrag gegen die bestehende Wormhole-Referenz-DLL.
add_executable(test_e2_contract_abi_vertrag "test_e2_contract_abi_vertrag.cpp")
target_compile_features(test_e2_contract_abi_vertrag PRIVATE cxx_std_23)
target_include_directories(test_e2_contract_abi_vertrag PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_e2_contract_abi_vertrag PRIVATE comdare::anatomy_module_loader)
comdare_set_platform_defines(test_e2_contract_abi_vertrag)
target_compile_definitions(test_e2_contract_abi_vertrag PRIVATE
    "COMDARE_E2_PILOT_DLL=\"$<TARGET_FILE:anatomy_codegen_pilot_wormhole_shared>\"")
add_dependencies(test_e2_contract_abi_vertrag anatomy_codegen_pilot_wormhole_shared)
add_test(NAME test_e2_contract_abi_vertrag COMMAND test_e2_contract_abi_vertrag)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_e2_contract_abi_vertrag)
set_tests_properties(test_e2_contract_abi_vertrag PROPERTIES LABELS "e2;abi")

# Querschnitt M - SystemAxis-Wurzel ("Blut") + CMD-1-b MeasurementVisitable, ohne DLL-/Umbrella-Bezug.
add_executable(test_m_contract_system_axis_wurzel "test_m_contract_system_axis_wurzel.cpp")
target_compile_features(test_m_contract_system_axis_wurzel PRIVATE cxx_std_23)
target_include_directories(test_m_contract_system_axis_wurzel PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_m_contract_system_axis_wurzel PRIVATE gtest gtest_main)
comdare_set_platform_defines(test_m_contract_system_axis_wurzel)
target_compile_definitions(test_m_contract_system_axis_wurzel PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_m_contract_system_axis_wurzel COMMAND test_m_contract_system_axis_wurzel)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m_contract_system_axis_wurzel)
set_tests_properties(test_m_contract_system_axis_wurzel PROPERTIES LABELS "m;blut")

# Limits-Entkopplung Vorstufe - XML-Profil generiert Build-Header, kein Produktivkonsum.
add_executable(test_limits_entkopplung_vorstufe
    "test_limits_entkopplung_vorstufe.cpp")
target_compile_features(test_limits_entkopplung_vorstufe PRIVATE cxx_std_23)
target_include_directories(test_limits_entkopplung_vorstufe PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_limits_entkopplung_vorstufe PRIVATE Boost::mp11)
comdare_set_platform_defines(test_limits_entkopplung_vorstufe)
target_compile_definitions(test_limits_entkopplung_vorstufe PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
comdare_attach_generated_catalog(test_limits_entkopplung_vorstufe)
add_test(NAME test_limits_entkopplung_vorstufe COMMAND test_limits_entkopplung_vorstufe
    "${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_limits_entkopplung_vorstufe)
set_tests_properties(test_limits_entkopplung_vorstufe PROPERTIES LABELS "e4;limits")

# Phase 7 (Parser-/Gating-Konsolidierung): Regressions-Gate der DOM-Fassung (parse_profile/
# load_messreihen/parse_one) gegen die eingefrorenen Regex-Referenzwerte.
add_executable(test_parser_konsolidierung "test_parser_konsolidierung.cpp")
target_compile_features(test_parser_konsolidierung PRIVATE cxx_std_23)
target_include_directories(test_parser_konsolidierung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization")
target_link_libraries(test_parser_konsolidierung PRIVATE comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_parser_konsolidierung)
add_test(NAME test_parser_konsolidierung COMMAND test_parser_konsolidierung
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/sota")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_parser_konsolidierung)
set_tests_properties(test_parser_konsolidierung PROPERTIES LABELS "parser;e4")

# Phase-6-Vorbau: IMeasurementSource (vendor-neutral) + Mess-Achsen-Registry + E4'-Kurven-Skeleton.
add_executable(test_phase6_vorbau "test_phase6_vorbau.cpp")
target_compile_features(test_phase6_vorbau PRIVATE cxx_std_23)
target_include_directories(test_phase6_vorbau PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
comdare_set_platform_defines(test_phase6_vorbau)
target_compile_definitions(test_phase6_vorbau PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_phase6_vorbau COMMAND test_phase6_vorbau)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_phase6_vorbau)
set_tests_properties(test_phase6_vorbau PROPERTIES LABELS "m;phase6")

# Phase 0.1 SIMD-Dispatch (Doc 21 §F): Amd64Isa::simd_field_sum dispatcht auf die aktive Vektor-Breite.
# Der Basis-Test baut mit den Plattform-Defaults (auf x86_64 = SSE2-Baseline). Damit die BREITEREN Dispatch-
# Pfade (AVX2 = 8 uint32/Iter, AVX-512F = 16 uint32/Iter) in der CI nicht nur latent bleiben, sondern ECHT
# kompiliert (Compile-Deckung aller #if-Zweige) und — wo die Host-ISA es zulaesst — auch AUSGEFUEHRT werden,
# erzeugen wir je ISA-Stufe eine eigene Test-Variante mit der passenden -m-Flag (Review wf_fd87be00: die
# Overflow-Build-Invarianz je Stufe muss real bewiesen werden, nicht nur der SSE2-Pfad). Getrennt gehalten:
#  * BAUEN immer (COMDARE_TEST_TARGETS) -> CI kompiliert alle Dispatch-Zweige, wenn der Compiler die Flag kennt.
#  * AUSFUEHREN nur, wenn der BUILD-Host die ISA zur Laufzeit unterstuetzt (Probe via __builtin_cpu_supports,
#    OHNE ISA-Flag kompiliert -> kann selbst nie SIGILLen). Cross-Compile: check_cxx_source_runs entfaellt ->
#    Variante wird nur kompiliert. So kein SIGILL auf Runnern ohne AVX2/AVX-512.
include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

check_cxx_source_runs("int main(){return __builtin_cpu_supports(\"avx2\")?0:1;}" COMDARE_HOST_RUNS_AVX2)
check_cxx_source_runs("int main(){return __builtin_cpu_supports(\"avx512f\")?0:1;}" COMDARE_HOST_RUNS_AVX512F)

function(comdare_add_simd_dispatch_test suffix flag do_run)
    set(_tgt "test_simd_field_sum_dispatch${suffix}")
    add_executable(${_tgt} "test_simd_field_sum_dispatch.cpp")
    target_compile_features(${_tgt} PRIVATE cxx_std_23)
    # Include-Konvention wie das Schwester-Target test_v41_axis_09b_simd_extension: die generierten
    # (configure_file) Header — allen voran axes/simd/axis_09_isa_flags.hpp — EXPLIZIT nachreichen, damit das
    # Target self-sufficient ist und nicht implizit auf comdare_set_platform_defines' generated-Kante baut.
    target_include_directories(${_tgt} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        "${PROJECT_BINARY_DIR}/generated/axes/simd"
        "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_09b_simd_extension"
        "${PROJECT_BINARY_DIR}/generated/topics/hardware/axis_12_general_hardware"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    comdare_set_platform_defines(${_tgt})
    if(NOT flag STREQUAL "")
        target_compile_options(${_tgt} PRIVATE ${flag})
    endif()
    # Immer in den Sammel-Bau (comdare_tests) + Codegen-Kante -> CI kompiliert diesen Dispatch-Zweig.
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_tgt})
    if(do_run)
        add_test(NAME ${_tgt} COMMAND ${_tgt})
        set_tests_properties(${_tgt} PROPERTIES LABELS "axes;simd")
    endif()
endfunction()

# Baseline (Plattform-Default: SSE2 auf x86_64, skalar sonst) — immer bauen UND ausfuehren.
comdare_add_simd_dispatch_test("" "" TRUE)

# AVX2-Variante: bauen, wenn der Compiler -mavx2 kennt; ausfuehren nur bei Host-AVX2.
check_cxx_compiler_flag("-mavx2" COMDARE_CXX_HAS_MAVX2)
if(COMDARE_CXX_HAS_MAVX2)
    comdare_add_simd_dispatch_test("_avx2" "-mavx2" ${COMDARE_HOST_RUNS_AVX2})
endif()

# AVX-512F-Variante: bauen, wenn der Compiler -mavx512f kennt; ausfuehren nur bei Host-AVX-512F.
check_cxx_compiler_flag("-mavx512f" COMDARE_CXX_HAS_MAVX512F)
if(COMDARE_CXX_HAS_MAVX512F)
    comdare_add_simd_dispatch_test("_avx512" "-mavx512f" ${COMDARE_HOST_RUNS_AVX512F})
endif()

# M-CE-16 (G3 Batch-2): Regressionstest für comdare::simd::avx2::add_u64 (Carry-Verlust + UB-Tail-Read).
# Wie der simd_field_sum-Dispatch: je ISA-Stufe eine Variante — die avx2-Variante (mit -mavx2) instanziiert
# GENAU die zuvor kaputte AVX2-Fassung und weist r[4]==13 nach. Gtest-basiert → gtest/gtest_main + comdare::simd.
function(comdare_add_simd_add_u64_test suffix flag do_run)
    set(_tgt "test_simd_add_u64_carry${suffix}")
    add_executable(${_tgt} "test_simd_add_u64_carry.cpp")
    target_compile_features(${_tgt} PRIVATE cxx_std_23)
    comdare_set_platform_defines(${_tgt})
    target_link_libraries(${_tgt} PRIVATE gtest gtest_main comdare::simd)
    if(NOT flag STREQUAL "")
        target_compile_options(${_tgt} PRIVATE ${flag})
    endif()
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_tgt})
    if(do_run)
        add_test(NAME ${_tgt} COMMAND ${_tgt})
        set_tests_properties(${_tgt} PROPERTIES LABELS "axes;simd" TIMEOUT 60)
    endif()
endfunction()

# Baseline (Plattform-Default) — immer bauen UND ausfuehren.
comdare_add_simd_add_u64_test("" "" TRUE)
if(COMDARE_CXX_HAS_MAVX2)
    comdare_add_simd_add_u64_test("_avx2" "-mavx2" ${COMDARE_HOST_RUNS_AVX2})
endif()
if(COMDARE_CXX_HAS_MAVX512F)
    comdare_add_simd_add_u64_test("_avx512" "-mavx512f" ${COMDARE_HOST_RUNS_AVX512F})
endif()

# thesis_tiere ADHOC-Treiber: baubar per explizitem Target, aber absichtlich
# nicht in ctest/COMDARE_TEST_TARGETS registriert.
# GRENZE (Review wf_1009d16f, empirisch): im super-Sub-Build sind diese EXCLUDE_FROM_ALL-Targets
# nicht baubar (COMDARE_AXIS_06_USE_MIMALLOC=1 dort, ADHOC erbt keine Vendor-Includes) — die
# Gates 4-5 laufen im ce-Standalone-Baum; der super-Beweis ist messung_driver ueber die Facade.
# AUFGELOEST (wf_1009d16f-FIX, 2026-07-12): Root-CMakeLists vererbt bei USE_MIMALLOC=1 das
# Vendor-Include+Archiv directory-scope (link_libraries(comdare::vendor_mimalloc) nach
# add_subdirectory(ext)) an ALLE Targets — auch diese ADHOC-Targets bauen jetzt im perms-ON-Baum.
set(_thesis_adhoc_includes
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})

set(_thesis_adhoc_libs
    Boost::mp11
    comdare_builder_xml_config_parser
    comdare::workload_driver
    comdare::anatomy_module_loader)

function(comdare_thesis_adhoc_executable target)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    add_executable(${target} EXCLUDE_FROM_ALL ${ARG_SOURCES})
    comdare_prepare_thesis_object_dir(${target})
    target_compile_features(${target} PRIVATE cxx_std_23)
    target_include_directories(${target} PRIVATE ${_thesis_adhoc_includes})
    target_link_libraries(${target} PRIVATE ${_thesis_adhoc_libs} ${ARG_LIBRARIES})
    comdare_set_platform_defines(${target})
    target_compile_definitions(${target} PRIVATE
        COMDARE_MEASUREMENT_ON=1
        COMDARE_CE_ENABLE_STATISTICS=1
        COMDARE_EXPERIMENT_MODE_ON=1)
    set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS ${target})
endfunction()

comdare_thesis_adhoc_executable(test_profile_roundtrip
    SOURCES "thesis_tiere/test_profile_roundtrip.cpp")

# M-CE-27 (Muster-F, 2026-07-13) — das volle golden-320-Roundtrip-Gate des PROFIL-Pfads (m3v2_study.profile.xml
# → parse_thesis_profile → StaticBinaryView == 320 binary_ids POSITIONSIDENTISCH zur eingefrorenen Golden-Liste;
# #139-Resume-Schutz). War bisher NUR als EXCLUDE_FROM_ALL-ADHOC baubar, ohne add_test / nicht im comdare_tests-
# Sammel-Bau → automatisiert geprueft war nur golden[0]. Der historische EXCLUDE_FROM_ALL-Blocker (super-Sub-Build-
# Vendor-Includes) ist laut Kopf-Kommentar AUFGELOEST (wf_1009d16f-FIX). Daher jetzt regulaer registriert:
# COMDARE_TEST_TARGETS zieht das EXCLUDE_FROM_ALL-Target per add_dependencies in den Sammel-Bau, add_test macht das
# Gate CI-lauffaehig. Profil-XML + Golden-ID-Liste werden als ABSOLUTE argv uebergeben (CWD-unabhaengig; beide nur
# GELESEN → golden_fullpilot_320 bleibt TABU-unberuehrt).
add_test(NAME test_profile_roundtrip
    COMMAND test_profile_roundtrip
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/m3v2_study.profile.xml"
        "${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_profile_roundtrip)
# M-CE-27 (Muster-F, 2026-07-15) — CI-Heimat: der dedizierte -R-Gate ist der contract:profile_coverage-Job der
# ce-.gitlab-ci.yml (Profil-/E4-Validierung, semantisch der golden/validation-Job). Das +contract-Label sorgt dafuer,
# dass test:unit (-LE contract|pmc) das Gate NICHT zusaetzlich sweept -> es laeuft in GENAU EINEM Job (Doktrin wie
# die M-CE-25-Profil-Gates). golden/roundtrip bleiben als Semantik-Label erhalten; comdare_tests baut es weiterhin mit.
set_tests_properties(test_profile_roundtrip PROPERTIES LABELS "thesis_tiere;golden;roundtrip;contract" TIMEOUT 300)

comdare_thesis_adhoc_executable(test_axis_sweep_pilot
    SOURCES "thesis_tiere/test_axis_sweep_pilot.cpp")

# ─────────────────────────────────────────────────────────────────────────────
# #25 B — Permutations-Codegen Byte-Identitaet (cpp-Tool == cmake-Backend; seit GO-2
# 2026-07-12 ist 'cpp' das Default-Backend, 'cmake' bleibt Referenz-Backend).
# Script-Test (CMake-Primitiv add_test, KEIN gtest-Binary): der Treiber
# perm_codegen_byte_identity.cmake faehrt codegen.cmake (cmake -P) UND das gebaute
# comdare_permutation_codegen_cli auf dieselben Eingaben und vergleicht die erzeugte
# permutations.cmake (+ manifest) byte-fuer-byte. Deckt smoke + medium ab.
# Voraussetzung: 'cmake --build <build> --target comdare_permutation_codegen_cli' (2-Pass; das
# Tool ist ein normales app-Target und wird vom Default-Build ohnehin gebaut).
# ─────────────────────────────────────────────────────────────────────────────
if(TARGET comdare_permutation_codegen_cli)
    foreach(_perm_profile smoke medium)
        add_test(
            NAME test_permutation_codegen_byte_identity_${_perm_profile}
            COMMAND "${CMAKE_COMMAND}"
                "-DCODEGEN_CMAKE=${PROJECT_SOURCE_DIR}/tools/permutation_codegen/codegen.cmake"
                "-DCLI_TOOL=$<TARGET_FILE:comdare_permutation_codegen_cli>"
                "-DAXES_VERSIONS=${PROJECT_SOURCE_DIR}/tools/permutation_codegen/axes_versions.txt"
                "-DPROFILE=${_perm_profile}"
                "-DTARGET_ISA=auto"
                "-DMODE=on_build_on_demand"
                "-DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/perm_byte_${_perm_profile}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/perm_codegen_byte_identity.cmake")
        set_tests_properties(test_permutation_codegen_byte_identity_${_perm_profile}
            PROPERTIES TIMEOUT 120)
    endforeach()
endif()
