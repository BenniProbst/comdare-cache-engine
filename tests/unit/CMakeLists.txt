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

# -----------------------------------------------------------------------------
# HAERTUNG Posten 69 (2026-08-04, Ledger-Nachtrag abend-10) -- per-BUILD-VERZEICHNIS-Kennung
# fuer die Test-Temp-Wurzel (comdare_test_tmp.hpp).
#
# BEFUND: die Wurzel war NUR per-UID benannt (#278/#24: temp_directory_path()/comdare_test_<uid>).
# Das loest die urspruengliche Klasse (FREMDE User auf Shell-Runnern) korrekt, aber NICHT den Fall,
# der uns real trifft: MEHRERE Worktrees DESSELBEN Users, deren ctest-Laeufe parallel fahren. Die
# teilen sich dann eine Wurzel (Ist-Erhebung: 15683 Eintraege) und ueberschreiben/loeschen einander
# die Fixture-Dateien -- Flake-Klasse ~27% an test_s5_artifact_cache_bounded/test_cache_mc_timeout.
#
# HEILUNG: die Wurzel traegt zusaetzlich eine STABILE Kennung des Build-Verzeichnisses. Sie kommt aus
# CMake (der einzige Ort, der das Build-Verzeichnis kennt) und ist ein MD5-Praefix ueber
# CMAKE_BINARY_DIR: deterministisch je Build-Verzeichnis (zwei Konfigurationen desselben Verzeichnisses
# liefern dieselbe Kennung -- KEIN Zeitstempel, keine Zufallszahl, kein Aufraeum-Bruch), und zwei
# Worktrees mit eigenen Build-Verzeichnissen bekommen zwangslaeufig verschiedene Wurzeln.
#
# WARUM VERZEICHNIS-WEIT statt je Ziel: die Konsumenten des Headers sind ueber DREI Registrier-Wege
# verteilt (comdare_add_test, die Phase-E-/G3-Listen-Schleifen und direkte add_executable-Bloecke).
# Eine handgepflegte Ziel-Liste driftet bei der naechsten neuen TU sofort ab; die Verzeichnis-Eigenschaft
# gilt fuer JEDES hier erzeugte Ziel und bleibt damit ohne Pflegeaufwand wahr. Auf Zielen, die den
# Header nicht ziehen, ist das Makro schlicht unbenutzt (kein emittiertes Byte aendert sich -- der
# golden-320-Roundtrip je Commit belegt es).
string(MD5 _comdare_test_tmp_dir_md5 "${CMAKE_BINARY_DIR}")
string(SUBSTRING "${_comdare_test_tmp_dir_md5}" 0 12 _comdare_test_tmp_build_tag)
add_compile_definitions(COMDARE_TEST_TMP_BUILD_TAG="${_comdare_test_tmp_build_tag}")
message(STATUS
    "Test-Temp-Wurzel (Posten 69): comdare_test_<uid>_${_comdare_test_tmp_build_tag} "
    "(stabil aus CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})")

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

# W0a/S4 (2026-08-10): HIER STAND EIN GATTER AUF ANWESENHEIT -- ersatzlos entfernt.
#
# Entfernt wurden (a) die INTERFACE-Library comdare_prt_art_concepts, deren
# target_include_directories auf "${PROJECT_SOURCE_DIR}/prt_art/include" zeigte,
# und (b) das Gatter
#     set(COMDARE_PRT_ART_LEGACY_AVAILABLE FALSE)
#     if(EXISTS "${PROJECT_SOURCE_DIR}/prt_art/include/prt_art/prt_art.hpp") ...
#
# WARUM BEIDES FIKTION WAR, am Objekt gemessen (10.08.2026):
#   prt_art.hpp: 0 Treffer in ce, 0 Treffer in comdare-prt-art, ueber die ganze
#   Historie beider Repos. Gegenprobe cache_engine.hpp: 1 Treffer (das Werkzeug
#   sucht). Der Pfad prt_art/ UNTER DER CE-WURZEL kann per Konstruktion nie
#   entstehen: der Plugin-Weg haengt Pruefling-Header ausschliesslich ueber
#   COMDARE_PRUEFLING_INCLUDE_DIRS (Geschwister-Pfad) an und kopiert nie etwas
#   unter die CE-Wurzel. Gemessen mit GELADENEM echtem Pruefling: die vier
#   gegatterten Tests erschienen in `ctest -N` 0 mal (Gegenprobe: der
#   Pruefling-Integrationstest 1 mal, test_pressure_state 1 mal).
#   Das Gatter war damit nicht "falsch konfiguriert", sondern unerfuellbar.
#
# DIE VIER .cpp-DATEIEN BLEIBEN LIEGEN. Ihre Disposition (neu verdrahten /
# archivieren / loeschen) ist Ledger PA-3 und gehoert dem Owner; sie war ohne
# diese Registrierung schon bisher ausserhalb jedes Bauwegs und bleibt es.
#
# ERSATZ ist keine zweite Pfad-Frage, sondern eine ZUSICHERUNG:
# cmake/pruefling_kontrakt.cmake -- der Pruefling erklaert, was er liefert.
include(registrierungs_protokoll)   # hochgezogen von :1104 -- der Block unten
                                    # liegt davor und koennte sonst nicht vermerken

comdare_pruefling_faehigkeit(pruefling_slots_v1 _pa_slots)
if(_pa_slots)
    get_property(_pa_namen GLOBAL PROPERTY COMDARE_PRUEFLING_NAMEN)
    # Die Testnamen stehen noch nicht als Ziele fest (der Registrierungs-foreach
    # laeuft weiter unten). Sie werden hier aus derselben Quelle abgeleitet, aus
    # der er sie ableitet -- NAME_WE der zugesicherten Testquellen.
    set(_pa_tests "")
    foreach(_pa_src IN LISTS COMDARE_PRUEFLING_TEST_SOURCES)
        get_filename_component(_pa_n "${_pa_src}" NAME_WE)
        list(APPEND _pa_tests "${_pa_n}")
    endforeach()
    if(NOT _pa_tests)
        set(_pa_tests "-")
    endif()
    comdare_registrierung_vermerken(BLOCK pruefling_slots_v1 STATUS AKTIV
        TESTS ${_pa_tests}
        GRUND "Pruefling ${_pa_namen} sichert pruefling_slots_v1 zu")
else()
    comdare_registrierung_vermerken(BLOCK pruefling_slots_v1 STATUS UEBERSPRUNGEN
        TESTS - GRUND "kein geladener Pruefling sichert pruefling_slots_v1 zu")
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

# W0a/S4: die drei Registrierungen test_concepts_compile / test_value_handle /
# test_three_layer_audit sind hier entfallen. Sie hingen an
# if(COMDARE_PRT_ART_LEGACY_AVAILABLE) -- einem Gatter, das per Konstruktion nie
# TRUE werden konnte (Begruendung oben). Die drei Quelldateien liegen weiter im
# Baum; ihre Disposition ist Ledger PA-3 (Owner). Sie gegen das heutige
# Plugin-Modell neu zu verdrahten waere falsch: ihre Includes
# (prt_art/concepts/..., prt_art/page_structures/...) existieren im heutigen
# Pruefling nicht -- gemessen 0 Treffer fuer NodeRefKind/Encoding/IteratorMode
# ausserhalb der Waisen selbst. Ein Gatter, das sie oeffnet, erzeugt einen
# Daueralarm, der am gesunden Objekt nie gruen wird (T-1).

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

# W0a/S4: die Registrierung test_six_page_structures ist hier entfallen (gleiches
# unerfuellbares Gatter, Begruendung oben). Die Quelldatei bleibt liegen -- PA-3.
#
# UND DIE ERSATZBEHAUPTUNG IST GESTRICHEN: hier stand, die 6 Seitentypen seien
# "aktuell durch test_v41_axis_01_page_type abgedeckt". Nachgeprueft und FALSCH.
# test_v41_axis_01_page_type deckt CE-eigene Seitentypen ab (DenseByte,
# ExtendedDense, SparsePatricia, Redirect, CustomCache, BPlus); die Waise prueft
# prt-arts Interpreter-Singletons mit eigenen SIMD-Flags. Die "6" ist die
# Uebereinstimmung zweier verschiedener Zaehl-Domaenen, kein Nachfolge-Beleg.
# Eine Fundstelle, die eine Abdeckung behauptet, die es nicht gibt, kostet mehr
# als gar keine Fundstelle.

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

# T-15 (2026-08-09) -- DAS GATE IN DER MESS-SCHLEIFE, nicht die Mechanik.
# SELBSTCHECK: test_chaos_drift_gate (#197, darueber) beweist den MECHANISMUS; dieser Test beweist,
# dass die produktive Zelle ihn ueberhaupt RUFT. Der Register-Befund S5-06 vom 09.08.2026 lautete
# "kein einziger realer Messwert laeuft heute durch das Drift-Gate" -- exakt 3 Nicht-Build-Dateien
# nannten run_with_drift_gate, 0 davon produktiv. Deterministisches ns-Skript, keine Zeitnahme, kein
# Zufall. COMDARE_T15_ITERATOR_QUELLE reicht die Iterator-Quelle fuer die Verdrahtungs-Wache durch
# (Muster von test_module_loader, das ebenfalls einen Pfad per compile definition erhaelt).
# Rezept gespiegelt von test_m1hb_observer_zellen_ehrlich (weiter unten): standalone int main(), das
# den Iterator-Header zieht -> derselbe Include-Satz und dieselben Mess-Defines. Sie beschreiben die
# uebersetzende TU, nicht die geprueften Zeilen.
add_executable(test_t15_drift_gate_messschleife "test_t15_drift_gate_messschleife.cpp")
target_compile_features(test_t15_drift_gate_messschleife PRIVATE cxx_std_23)
target_include_directories(test_t15_drift_gate_messschleife PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_t15_drift_gate_messschleife PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_t15_drift_gate_messschleife)
# SELBSTCHECK (09.08.2026): der Iterator-Pfad steht ueber eine Zwischenvariable, NICHT ausgeschrieben.
# Ausgeschrieben war die Definitions-Zeile 135 Byte lang und die DIFF-HYGIENE-WACHE riss sie
# (ci_diff_ascii_width_guard.sh, ce-Pipeline 15427, ">120-SPALTEN tests/unit/CMakeLists.txt:254").
# Wert-identisch: _t15_tree + "/cache_engine_builder_iterator.hpp" ergibt denselben Pfad wie zuvor.
set(_t15_tree "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/experiment_tree")
target_compile_definitions(test_t15_drift_gate_messschleife PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_T15_ITERATOR_QUELLE=\"${_t15_tree}/cache_engine_builder_iterator.hpp\""
    "COMDARE_T15_FASSADE_QUELLE=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade/profile_run_entry.hpp\"")
unset(_t15_tree)
add_test(NAME test_t15_drift_gate_messschleife COMMAND test_t15_drift_gate_messschleife)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_t15_drift_gate_messschleife)
set_tests_properties(test_t15_drift_gate_messschleife PROPERTIES LABELS "t15;drift;messschleife;standalone")

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

# K7b-1 (Section 62-B / Section 64) consteval SHA-512 fuer den Anatomy-Stempel-Fingerprint (6. AnatomyVersionLines-Feld)
comdare_add_test(test_ctsha512
    SOURCES test_ctsha512.cpp)
target_include_directories(test_ctsha512 PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})

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
# A8-S5-02a (2026-08-04): der Stream-Puffer laeuft seit dem Schnitt ueber die Allokator-ACHSE
# (axis_06_allocator_exgen). Damit zieht diese TU die Achsen-Include-Kette (generierte Flags-Header,
# measurement/measurable_concept.hpp aus libs/cache_engine/src, Boost.MP11) und braucht denselben
# Include-Satz wie die uebrigen Achsen-TUs -- der Satz ist von den Phase-E-Tests uebernommen, nicht erfunden.
comdare_add_test(test_v41_serialization_primitives
    SOURCES test_v41_serialization_primitives.cpp)
target_include_directories(test_v41_serialization_primitives PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_v41_serialization_primitives PRIVATE Boost::mp11)

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
# D2 (09.08.2026): jeder bedingte Registrierungs-Block vermerkt sich im
# Registrierungs-Protokoll -- BEIDE Zweige. Die Abdeckungs-Wache liest es und
# macht einen uebersprungenen Block ROT, statt ueber einem stillschweigend
# kleineren Nenner Vollstaendigkeit zu melden (cmake/registrierungs_protokoll.cmake).
#
# W0a/S4 (10.08.2026): dieser include ist seit dem Hochziehen an den Dateikopf ein
# No-op und bleibt als Lesehilfe stehen (Hausmuster, vgl. das zweite
# enable_testing() in der Root-CMakeLists). Er MUSSTE hochgezogen werden: die
# bedingten Bloecke am Dateianfang liegen davor und koennten comdare_registrierung_
# vermerken() sonst gar nicht aufrufen -- ein Block, der sich nicht melden kann,
# ist fuer die Abdeckungs-Wache unsichtbar. Die Init ist per GLOBAL PROPERTY
# geschuetzt, ein zweiter include schreibt das Protokoll daher nicht neu.
include(registrierungs_protokoll)
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
    comdare_registrierung_vermerken(
        BLOCK  r5i_configure_codegen
        STATUS AKTIV
        TESTS  test_v41_anatomy_r5i_configure_codegen
        GRUND  "anatomy-codegen-Werkzeug gefunden (status=${_r5i_status}), Snippet vorhanden")
else()
    message(STATUS
        "R5.I: configure-time codegen SKIPPED (status=${_r5i_status}). "
        "Build the tool first: cmake --build . --target comdare_anatomy_codegen_cli, "
        "then re-configure to enable the R5.I pilot test.")
    comdare_registrierung_vermerken(
        BLOCK  r5i_configure_codegen
        STATUS UEBERSPRUNGEN
        TESTS  test_v41_anatomy_r5i_configure_codegen
        GRUND  "status=${_r5i_status}: comdare_anatomy_codegen_cli bauen, dann reconfigure")
endif()

# V41.F.6.1 F.5 — comdare_perms_pa Pilot: Pruefling-Stufe-2-Composition -> DLL (Plugin-Controller).
# MATERIALISIERT die prt-art-Composition als Permutations-Binary (Beweis "Binaries aus Rekombinationen").
# Nur aktiv wenn ein Pruefling geladen ist (COMDARE_PRUEFLING_INCLUDE_DIRS via COMDARE_CE_PRUEFLINGE)
# UND das Codegen-Tool bereits gebaut wurde (2-Pass-Build, wie R5.I).
#
# D2-AUSNAHME, BENANNT STATT VERSCHWIEGEN: die beiden Bloecke unten (comdare_perms_pa,
# comdare_perms_full_join) stehen BEWUSST NICHT im Registrierungs-Protokoll. Sie
# registrieren keinen einzigen ctest-Test, sondern nur DLL-Targets -- sie koennen den
# Nenner der Abdeckungs-Wache also gar nicht verkleinern. Wer hier spaeter ein
# comdare_add_test() einfuegt, muss den Block nachtragen; der Protokoll-Aufruf gehoert
# dann in BEIDE Zweige (siehe R5.I/F15 oben).
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
    comdare_registrierung_vermerken(
        BLOCK  f15_measurement
        STATUS AKTIV
        TESTS  test_v41_anatomy_f15_measurement
        GRUND  "anatomy-codegen-Werkzeug gefunden (status=${_f15_status}), Snippet vorhanden")
else()
    message(STATUS
        "F15: SKIPPED (status=${_f15_status}). Build comdare_anatomy_codegen_cli first, then re-configure.")
    comdare_registrierung_vermerken(
        BLOCK  f15_measurement
        STATUS UEBERSPRUNGEN
        TESTS  test_v41_anatomy_f15_measurement
        GRUND  "status=${_f15_status}: comdare_anatomy_codegen_cli bauen, dann reconfigure")
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

# D5-1 PERZENTIL-KANON (2026-08-09) -- die EINE Perzentil-Definition (stats::nearest_rank_index,
# Lehrbuch-Formel ceil(q*n)-1) + die Wache darueber. Die drei COMDARE_D51_ZENSUS_ROOT_*-Defines sind
# QUELLPFADE, die der Test NUR LIEST (gleiches Muster wie COMDARE_A8S4_ABI_ADAPTER_SRC):
#   _BUILDER/_HARNESS = Auswertungs-Baum, in dem genau EINE Quantil->Index-Umrechnung stehen darf;
#   _CE               = der ganze ce-Baum, in dem der Name nearest_rank_p 0-mal vorkommen muss.
# Link auf workload_driver, weil der Kreuz-Test die ECHTEN Serialisierer fahrt (kein Nachbau).
comdare_add_test(test_d51_perzentil_kanon
    SOURCES test_d51_perzentil_kanon.cpp)
target_include_directories(test_d51_perzentil_kanon PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_d51_perzentil_kanon PRIVATE
    comdare::workload_driver)
target_compile_definitions(test_d51_perzentil_kanon PRIVATE
    "COMDARE_D51_ZENSUS_ROOT_BUILDER=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/builder\""
    "COMDARE_D51_ZENSUS_ROOT_HARNESS=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/harness\""
    "COMDARE_D51_ZENSUS_ROOT_CE=\"${PROJECT_SOURCE_DIR}/libs/cache_engine\"")

# D5-4 TRACE-SCHEMA (2026-08-09) -- die EINE Feldliste der Tier-Trace-Ausgabe (trace_schema::kLatenzFelder),
# BEIDSEITIG geprueft: Schema -> Ausgabe (alle fuenf Serialisierer, Wert-Pins von Hand) und Ausgabe ->
# Schema (Form-Suche <praefix>_p<ziffern>_ns findet keine Waise). Reine Header (anatomy_commands +
# latency_stats) -> es genuegt der ce-Include-Pfad, kein Lib-Link.
comdare_add_test(test_d54_trace_schema
    SOURCES test_d54_trace_schema.cpp)
target_include_directories(test_d54_trace_schema PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# PAKET W3-C (Ledger Sec.32-F8): Heuristik-Spline-Vorbau -- AxisSpline (Strategy: Fritsch-Carlson monoton /
# natuerlich-kubisch) + Break-Even-Finder + n/a-toleranter Measurement-Curve-Loader. Header-only, keine
# Lib-Abhaengigkeit -> nur der ce-Include-Pfad noetig.
# T-9 (2026-08-07): dazu der Min/Max-Katalog (heuristik/axis_optimization_catalog.hpp) und sein Drift-Beleg
# gegen kCompositionAxisNames. Der Beleg liest axis_path_serialization.hpp, und DAS ist der einzige Grund
# fuer Boost::mp11 an diesem Target -- der Katalog-Header selbst bleibt bewusst boost-frei (reine std).
comdare_add_test(test_heuristik_spline_break_even
    SOURCES test_heuristik_spline_break_even.cpp)
target_include_directories(test_heuristik_spline_break_even PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_heuristik_spline_break_even PRIVATE Boost::mp11)

# T-10 (2026-08-07): Workload-Cluster-Bildung, OFFLINE-Haelfte (Merkmalsvektor + Normalisierung +
# agglomerative Hierarchie + k-Means-Verfeinerung + k-Wahl-Diagnostik). Plan-Quelle: super
# docs/audits/20260709-workload-cluster-research-BEFUND.md. Header-only, reine std -> KEIN Boost.
# Der Test belegt zusaetzlich die Concept-Naht gegen das reale WorkloadProfileAggregate aus
# builder/cacheline_policy/cacheline_policy_selector.hpp -> Include-Root libs/cache_engine.
comdare_add_test(test_heuristik_workload_cluster
    SOURCES test_heuristik_workload_cluster.cpp)
target_include_directories(test_heuristik_workload_cluster PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

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
# Hermetik (CI-3): scan_range zieht transitiv legacy_original_code_strategy_concept.hpp (boost::mp11).
# Die vendored boost/mp11.hpp ist ein 23-Zeilen-Fassade-Redirect; nur die Boost::mp11-INTERFACE
# traegt den korrekten vendored Include-Pfad. Ohne Link maskiert lokal ein transitiver Pfad den
# Fehler, in der hermetischen CI schlaegt der Compile fehl ("'boost' has not been declared").
target_link_libraries(test_v41_scan_range_organ PRIVATE Boost::mp11)

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

# F5.R1 (#35) — ICacheEngine Master-Framework-Gesamt-Fassade: Smoke-Test (get_cache_engine() über die EINE Tür;
# measurement/isa_dispatch/build_tools/test_system real vermittelt, search_engine/core ehrlich DEFERRED-#274).
comdare_add_test(test_v41_cache_engine_facade
    SOURCES test_v41_cache_engine_facade.cpp
    LIBRARIES comdare::workload_generator)
target_include_directories(test_v41_cache_engine_facade PRIVATE
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

# S3 (§62-B COMDARE_PRUEF_ONLY): run_so_conformance_gate laedt die r5g-adhoc-.so-Fixture + faehrt NUR das Gate.
comdare_add_test(test_pruef_only_gate
    SOURCES test_pruef_only_gate.cpp)
target_include_directories(test_pruef_only_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_pruef_only_gate PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
target_compile_definitions(test_pruef_only_gate PRIVATE
    "COMDARE_R5G_ADHOC_DLL_DIR=\"${_r5g_adhoc_dir}\"")
add_dependencies(test_pruef_only_gate comdare_r5g_adhoc_perm)

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
        comdare_registrierung_vermerken(
            BLOCK  r5g_adhoc_autobuilt
            STATUS AKTIV
            TESTS  test_v41_anatomy_adhoc_autobuilt_load
            GRUND  "comdare-adhoc-emitter gefunden, ${_r5g_autobuilt_count} Permutations-DLLs emittiert")

        # V41.F.6.1 R5.D — Smoke-Test des comdare-f15-compare-CLI ueber die realen R5.G-DLLs:
        # faehrt die echte Binary (nicht nur die Treiber-Logik) und prueft BEIDES --
        # Exit-Code 0 UND die "Ranking (schnellste"-Zeile in der Ausgabe.
        # (apps/ wird vor tests/ verarbeitet -> Target existiert.)
        #
        # D1e (2026-08-09) -- WARUM HIER EIN TREIBER STEHT UND KEIN PASS_REGULAR_EXPRESSION.
        # Bis heute stand hier `PASS_REGULAR_EXPRESSION "Ranking .schnellste"`. Diese
        # ctest-Eigenschaft VERWIRFT den Exit-Code vollstaendig: trifft der Regex, gilt der
        # Test als bestanden, auch wenn die Binary mit != 0 endet. Das war kein theoretisches
        # Risiko -- main.cpp hat 11 von 12 numerischen return-Anweisungen ungleich 0, und DREI
        # davon (return 4 in den Zeilen 549/558/566, CSV-/JSON-Schreibfehler) liegen HINTER der
        # Ranking-Zeile (481). Am Objekt nachgestellt: mit einem unbeschreibbaren --csv-Pfad
        # endet die echte Binary mit Exit 4, druckt die Ranking-Zeile -- und ctest meldete
        # "Passed  100% tests passed, 0 tests failed out of 1".
        # Der Kommentar versprach ausserdem eine "win_rate"-Ausgabe (main.cpp:455) und ein
        # "Exit-0"; geprueft wurde in Wahrheit nur die Ranking-Zeile (main.cpp:481).
        #
        # ABGRENZUNG: bei den drei Negativ-Compile-Fixturen weiter unten (E24-C1/C4, HY-A1)
        # bleibt PASS_REGULAR_EXPRESSION RICHTIG -- dort MUSS der Bau fehlschlagen, das
        # Verwerfen des Exit-Codes ist dort der Zweck. Hier ist es der Riss.
        if(TARGET comdare_f15_compare_cli)
            add_test(NAME f15_compare_cli_smoke
                COMMAND "${CMAKE_COMMAND}"
                    "-DCLI=$<TARGET_FILE:comdare_f15_compare_cli>"
                    "-DCLI_ARGS=${_r5g_autobuilt_dir};--batches=16;--alpha=0.05"
                    "-DEXPECT=Ranking .schnellste"
                    "-DLABEL=comdare-f15-compare"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/cli_smoke.cmake")
            # DIE BAUKANTE: ohne diese Zeile haengt die CLI an KEINEM Sammel-Target --
            # comdare_tests baut sie nie, und der Test faellt in der CI als "Not Run" auf
            # (#278 Klasse-B-Fix, dieselbe Klasse). Registriert wird das BUILD-TARGET,
            # nicht der Testname -- hier weichen sie als einzige Stelle der Datei ab.
            set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS comdare_f15_compare_cli)
            # TIMEOUT: comdare_add_test setzt 60s automatisch (cmake/gtest_setup.cmake:90);
            # ein rohes add_test bekommt NICHTS -- ein haengender Mess-Lauf blockierte den
            # Job bis zum globalen CI-Timeout. 300s, weil der Lauf 48 DLLs vermisst.
            set_tests_properties(f15_compare_cli_smoke PROPERTIES
                TIMEOUT 300
                LABELS "r5d;f15;cli")
            comdare_registrierung_vermerken(
                BLOCK  r5d_f15_compare_cli_smoke
                STATUS AKTIV
                TESTS  f15_compare_cli_smoke
                GRUND  "Target comdare_f15_compare_cli vorhanden")
        else()
            comdare_registrierung_vermerken(
                BLOCK  r5d_f15_compare_cli_smoke
                STATUS UEBERSPRUNGEN
                TESTS  f15_compare_cli_smoke
                GRUND  "Target comdare_f15_compare_cli existiert in dieser Konfiguration nicht")
        endif()
    else()
        # Der Emitter lief, hat aber NICHTS emittiert -- eigener Zweig, eigener Vermerk.
        # Sonst faellt genau der Fall durch, in dem das Werkzeug DA ist und trotzdem
        # kein Test entsteht (leerer Permutations-Raum, Glob greift ins Leere).
        message(STATUS
            "R5.G: Auto-Permutations-Skalierung SKIPPED (Emitter lief, 0 DLLs).")
        comdare_registrierung_vermerken(
            BLOCK  r5g_adhoc_autobuilt
            STATUS UEBERSPRUNGEN
            TESTS  test_v41_anatomy_adhoc_autobuilt_load
            GRUND  "Emitter gefunden, aber 0 Permutations-DLLs in ${_r5g_autobuilt_dir}")
        comdare_registrierung_vermerken(
            BLOCK  r5d_f15_compare_cli_smoke
            STATUS UEBERSPRUNGEN
            TESTS  f15_compare_cli_smoke
            GRUND  "haengt an r5g_adhoc_autobuilt -- dort 0 Permutations-DLLs")
    endif()
else()
    message(STATUS
        "R5.G: Auto-Permutations-Skalierung SKIPPED (status=${_r5g_ae_status}). "
        "Build comdare_adhoc_emitter_cli first, then re-configure.")
    comdare_registrierung_vermerken(
        BLOCK  r5g_adhoc_autobuilt
        STATUS UEBERSPRUNGEN
        TESTS  test_v41_anatomy_adhoc_autobuilt_load
        GRUND  "status=${_r5g_ae_status}: comdare_adhoc_emitter_cli bauen, dann reconfigure")
    comdare_registrierung_vermerken(
        BLOCK  r5d_f15_compare_cli_smoke
        STATUS UEBERSPRUNGEN
        TESTS  f15_compare_cli_smoke
        GRUND  "haengt an r5g_adhoc_autobuilt -- Emitter-Werkzeug fehlt (status=${_r5g_ae_status})")
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

# (a2) Abdeckungs-Gate fuer die Allokator-DOKU-AKTEN (2026-08-07). Eigener Block statt LIGHT_DTESTS-Liste,
# weil der Test den Aktenpfad als Compile-Define braucht (Muster: test_25_kanon_dataset_akten). Die 23 Akten
# in algorithm_profiles/allocators/ haben laut ihrem eigenen Kopf-Kommentar BEWUSST keinen Produktions-Leser
# (#48 Scheibe 2: "kein Code-Leser; Allocator-Wahl = CT-Adapter, Statischer-Dispatch-Doktrin") -- genau darum
# kann ihr Verfall heute unbemerkt bleiben, obwohl die Thesis-Tabelle tab:allocator-profiles zehn von ihnen
# namentlich zitiert. Kein Boost, keine generated-Flags: nur der self-contained KF-1-Reader aus libs/common.
add_executable(test_allocator_profile_bestand "test_allocator_profile_bestand.cpp")
target_compile_features(test_allocator_profile_bestand PRIVATE cxx_std_23)
target_include_directories(test_allocator_profile_bestand PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/common")
comdare_set_platform_defines(test_allocator_profile_bestand)
target_compile_definitions(test_allocator_profile_bestand PRIVATE
    "COMDARE_CE_ALLOCATOR_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/allocators\""
    "COMDARE_CE_ADAPTERS_DIR=\"${PROJECT_SOURCE_DIR}/adapters\"")
add_test(NAME test_allocator_profile_bestand COMMAND test_allocator_profile_bestand)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_allocator_profile_bestand)
set_tests_properties(test_allocator_profile_bestand PROPERTIES LABELS "goal_v6;phase_d")

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
    test_axis_view_policies                   # L-76c View-Achsen extent/layout/accessor (mdspan-Policies + Registries)
    test_e24_c0_organ_concept                 # E-24 C0 (M0-Vorstufe): OrganConcept gegen alle 5 Gattungs-Anatomien
    test_e24_c1_organ_concept                 # E-24 C1: Op-Familie (Gate-G2-Matrix) + ObservableAxis-Forwarding
    # E-24 C2 (Bauplan Paragraf 3.1-C2): je Engine EINE Test-TU fuer for_each_abi_adapter. Gate Paragraf 4.2:
    # Materialisierungs-Zaehlung == Slot-Permutations-Soll des Genus (Soll fixture-unabhaengig hergeleitet).
    test_e24_c2_set_permutation_engine        # E-24 C2: SetPermutationEngine::for_each_abi_adapter (13 Slots)
    test_e24_c2_sequence_permutation_engine   # E-24 C2: SequencePermutationEngine::for_each_abi_adapter (9 Slots)
    test_e24_c2_view_permutation_engine       # E-24 C2: ViewPermutationEngine::for_each_abi_adapter (5 Slots)
    test_e24_c2_adapter_permutation_engine    # E-24 C2 (OP-3): NEUE AdapterPermutationEngine, 11 Slots
    # E-24 C3 (Bauplan Paragraf 3.1-C3): je Genus EINE Test-TU fuer Produktionstiefe. Gate Paragraf 4.2:
    # Organ-Member-Praesenz-Asserts gegen axis_names() (Liste gegen Liste) + Slot-Pins bleiben gruen.
    test_e24_c3_set_anatomy                   # E-24 C3: SetAnatomy 13 Organ-Member + observe_axes + Ctx
    test_e24_c3_sequence_anatomy              # E-24 C3: SequenceAnatomy 9 Organ-Member + observe_axes + Ctx
    test_e24_c3_view_anatomy                  # E-24 C3: ViewAnatomy 5 Organ-Member + observe_axes + Ctx
    test_e24_c3_adapter_anatomy               # E-24 C3: AdapterAnatomy 11 Organ-Member + observe_axes + Ctx
    test_e24_c3_cross_genus                   # E-24 C3 (L7-Rest): die 4 produktiven Cross-Genus-Verdrahtungen
    # E-24 C4 (Bauplan Paragraf 3.1-C4): Docks in-process produktionsreif. Gate Paragraf 4.1 (Klasse I) +
    # 4.2: die Orakel muessen in BEIDE Richtungen wirken -- reale Adapter bestehen, defekte Huellen fallen.
    test_e24_c4_genus_conformance_gates       # E-24 C4: Orakel je Genus (std::set/deque/queue+stack/span)
    # E-24 C5 (Bauplan Paragraf 6.2, FK-8): die Gattungs-Baupfad-D1-Meldung. Gate Paragraf 4.2 (Klasse II):
    # Count-Wachen im SELBEN Commit wie die Enum-Erweiterung + Token-Disjunktheits-Wache.
    test_e24_c5_fk8_genus_baupfad_d1          # E-24 C5: D1-Gate je Gattungs-Baupfad (Bindung + Aritaet)
    # A8-S3 (2026-08-04, Katalog-Arbeitsliste P1): die SearchAlgorithm-Anatomie haelt seit dieser Scheibe
    # ALLE 18 Achsen als Organ-Member (vorher 9 -- die uebrigen blieben eine stille 0). Die Wache zaehlt die
    # Accessoren und prueft gegen kV3AxisCount (nie gegen ein Literal) und belegt beide Ausgaenge: ehrlich
    # EmptyAxisSnapshot bei nackter Registry-Strategie, ECHTE Werte bei einem Cross-Genus-Sub-Organ.
    test_a8s3_sa_organ_member_vollstaendigkeit) # A8-S3: 18/18 SA-Organ-Member + Cross-Genus-SA-Naht
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

# -----------------------------------------------------------------------------
# S-14a BUMP-WACHE TEIL 1 -- DER RIEGEL (Task #33; Plan 13.3 #90; Owner KON17-03). Tripwire-ctest ueber
# das EXCLUDE_FROM_ALL-Werkzeug comdare_axis_version_lock: der Test bekommt Binary-Pfad und Quellbaum als
# argv, kopiert die Traeger-Baeume in ein Temp-Verzeichnis (comdare_test_tmp.hpp) und faehrt --write/--check
# NUR auf den Kopien (Koeder A-E; golden/TABU unberuehrt). 2-Pass-Muster der Tool-Gates
# (Vorbild f15_compare_cli_smoke): das Tool-Target haengt als add_dependencies-Kante am Test UND steht in
# COMDARE_TEST_TARGETS, damit der comdare_tests-Sammel-Bau es baut (sonst 'Not Run' in der CI, Klasse #278).
# SCHWERE TU mit Absicht (Blockvorbild test_reflect_versions_all17): sie loggt den CT-Zaehler
# kAllRegisteredOrganVariantCount NEBEN den Datei-Zaehlern des Tools -- nur loggen, nie gleichsetzen
# (Datei != Variante).
add_executable(test_s14_axis_version_lock_tripwire "test_s14_axis_version_lock_tripwire.cpp")
target_compile_features(test_s14_axis_version_lock_tripwire PRIVATE cxx_std_23)
target_include_directories(test_s14_axis_version_lock_tripwire PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s14_axis_version_lock_tripwire PRIVATE Boost::mp11)
comdare_set_platform_defines(test_s14_axis_version_lock_tripwire)
target_compile_definitions(test_s14_axis_version_lock_tripwire PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_dependencies(test_s14_axis_version_lock_tripwire comdare_axis_version_lock)
add_test(NAME test_s14_axis_version_lock_tripwire COMMAND test_s14_axis_version_lock_tripwire
    "$<TARGET_FILE:comdare_axis_version_lock>" "${PROJECT_SOURCE_DIR}")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_s14_axis_version_lock_tripwire)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS comdare_axis_version_lock)
# Raw add_test bekommt KEINEN Auto-Timeout (cmake/gtest_setup.cmake setzt ihn nur fuer comdare_add_test);
# 900s fuer Baum-Kopie (~8 MB seit Fixup 3: topics/ GANZ + anatomy/ + include/cache_engine/measurement/)
# + ~90 Tool-Laeufe mit SHA-256 ueber 718 Dateien je Lauf (Pflicht-Fixup 3 13.08.2026: Overlay-
# Grundgesamtheit 712 + heuristik 6, Koeder T1-T5 kamen hinzu; Debug-Builds rechnen die SHA deutlich
# langsamer als Release).
set_tests_properties(test_s14_axis_version_lock_tripwire PROPERTIES
    TIMEOUT 900
    LABELS "s14;axis_lock;tripwire")

# -----------------------------------------------------------------------------
# E-24 C6-V (2026-08-04, Entscheid E5 LEDGER:3828) -- die Statistics-Huellen der GATTUNGS-EIGENEN
# Achsen-Themen. Die Huellen sind COMDARE_CE_ENABLE_STATISTICS-gegated (Praezedenz
# axis_01_index_organization_observable.hpp:126-133), und diese TUs pruefen den EINGESCHALTETEN Zustand.
# EHRLICHE EINORDNUNG des Defines: es kommt am Ist ohnehin GLOBAL aus dem Root-CMakeLists
# (option(COMDARE_CE_ENABLE_STATISTICS ... ON) + add_compile_definitions, CMakeLists.txt:156-161) --
# die Zeile hier ist also REDUNDANT-ABSICHTLICH: sie haelt diese TUs auch dann auf dem ON-Pfad, wenn
# jemand die globale Option ausschaltet (dann wuerden sie sonst still den OFF-Pfad testen und ihre
# Aussage verlieren). Eigene Liste statt COMDARE_GOALV6_BOOST_DTESTS, weil dessen foreach keine
# per-Target-Definitionen zulaesst. Bau sonst identisch (plain int main(), Boost::mp11 + generated).
# OFFEN (kein C6-V-Gegenstand): der OFF-Pfad der Huellen wird in dieser Konfiguration von KEINER TU
# uebersetzt -- dieselbe Lage wie bei allen bestehenden Observable-Huellen des Repos.
set(COMDARE_E24_C6V_TESTS
    test_e24_c6v_growth_statistics       # C6-V (a): GrowthStatistics + ObservableGrowth (Sequence-Achse)
    test_e24_c6v_view_statistics         # C6-V (b): Extent/Layout/AccessorStatistics + Huellen (View-Achsen)
    test_e24_c6v_inner_container_statistics # C6-V (c): InnerContainerStatistics + Huelle (Adapter-Achse)
    test_e24_c6v_set_ehrlichkeit)        # C6-V (d): Set-Ehrlichkeits-Schliessung insert-/erase_attempt
foreach(_c6v IN LISTS COMDARE_E24_C6V_TESTS)
    add_executable(${_c6v} "${_c6v}.cpp")
    target_compile_features(${_c6v} PRIVATE cxx_std_23)
    target_include_directories(${_c6v} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_c6v} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_c6v})
    target_compile_definitions(${_c6v} PRIVATE COMDARE_CE_ENABLE_STATISTICS=1)
    add_test(NAME ${_c6v} COMMAND ${_c6v})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_c6v})
    set_tests_properties(${_c6v} PROPERTIES LABELS "goal_v6;phase_d;e24;c6v")
endforeach()

# -----------------------------------------------------------------------------
# E-24 C6/C7 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- die per-Achsen-WIRE-Formen der vier Container-
# Gattungs-Ebenen (XxxObserverAggregate<N>) + ihre eigenen Abfrage-Sub-Interfaces. Eigene Liste (nicht
# COMDARE_E24_C6V_TESTS), weil C6-V der ABI-NEUTRALE Vor-Baustein war und C6 die deklarierte ABI-Serie
# ist -- die Label-Trennung haelt beide im ctest getrennt filterbar (`ctest -L c6` vs. `-L c6v`).
# Bau identisch zum C6-V-Block (plain int main(), Boost::mp11 + generated, STATISTICS-ON-Pfad).
set(COMDARE_E24_C6_TESTS
    test_e24_c6_genus_observer_aggregate # C6 (a): das gemeinsame Wire-Layout GenusObserverAggregate<G, N>
    test_e24_c6_axis_row_writer          # C6 (b): write_genus_axis_row + E1-Milli-Fixpunkt
    test_e24_c6_set_wire_aggregate       # C6 (c): SetObserverAggregate<13> + ISetTierV2
    test_e24_c6_sequence_wire_aggregate  # C6 (d): SequenceObserverAggregate<9> + E1-Detail growth_factor_milli
    test_e24_c6_adapter_wire_aggregate   # C6 (e): AdapterObserverAggregate<11> + Adapter-Symmetrie
    test_e24_c6_view_wire_aggregate      # C6 (f): ViewObserverAggregate<5> + Elisions-/Layout-Beweis
    test_e24_c6_set_algebra_abi          # C6 (g) / F2: ISetAlgebraTier gegen das std::set-Orakel
    test_e24_c7_gattung_map_umbenennung  # C7-1: Ebene-1-Umbenennung SearchAlgorithm -> Map
    test_e24_c7_container_gattungs_kern   # C7-2: der Container-Gattungs-Kern + gestufter clear-Block
    test_e24_c7_element_type_vertrag      # C7-3: der Gattungs-Typ-Vertrag element_type
    test_e24_c7_gattung_abi_sichtbar      # C7-6: Ebene-1-Sichtbarmachung ohne neuen Wire-String
    test_e24_c7_symmetrie_und_benennung)  # C7-4/5/7: Map-Kern benannt, Set K=V, gattung-Symmetrie
foreach(_c6 IN LISTS COMDARE_E24_C6_TESTS)
    add_executable(${_c6} "${_c6}.cpp")
    target_compile_features(${_c6} PRIVATE cxx_std_23)
    target_include_directories(${_c6} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_c6} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_c6})
    target_compile_definitions(${_c6} PRIVATE COMDARE_CE_ENABLE_STATISTICS=1)
    add_test(NAME ${_c6} COMMAND ${_c6})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_c6})
    set_tests_properties(${_c6} PROPERTIES LABELS "goal_v6;phase_d;e24;c6;c7")
endforeach()

# -----------------------------------------------------------------------------
# E-24 C1 (Luecke L6) -- NEGATIV-COMPILE-FIXTURE: eine TU, die BRECHEN MUSS.
#
# Das Repo kannte am Ist KEIN Muster fuer erwartete Uebersetzungs-Fehler (Erhebung 04.08.: 0 Treffer
# fuer try_compile / WILL_FAIL unter tests/ und cmake/). Dieses Muster wird deshalb hier NEU eingefuehrt --
# auf dem offiziellen CMake/ctest-Weg und ausdruecklich benannt, damit Geschwister-Fixtures (C2/C3) es
# uebernehmen koennen statt es erneut zu erfinden:
#   (a) Das Ziel ist EXCLUDE_FROM_ALL und steht bewusst NICHT in COMDARE_TEST_TARGETS. Der Sammel-Bau
#       comdare_tests -- und damit die CI -- darf es NIE bauen; es muss ja fehlschlagen.
#   (b) Der ctest-Test RUFT den Bau ("cmake --build --target") und prueft die erwartete Diagnose ueber
#       PASS_REGULAR_EXPRESSION. Baut die TU versehentlich DURCH, fehlt der Marker -> Test ROT.
#   (c) Bewusst NICHT WILL_FAIL: das wuerde JEDEN Fehlschlag akzeptieren -- auch einen Tippfehler oder
#       einen fehlenden Include. Gefordert ist der BESTIMMTE Bruch, nicht irgendeiner.
#   (d) RESOURCE_LOCK serialisiert den geschachtelten Bau gegen kuenftige Geschwister-Fixtures (ein
#       zweiter gleichzeitiger Ninja-Aufruf auf demselben Bau-Verzeichnis wuerde blockieren).
set(_comdare_negativ_compile_tu test_e24_c1_organ_concept_negativ)
add_executable(${_comdare_negativ_compile_tu} EXCLUDE_FROM_ALL "${_comdare_negativ_compile_tu}.cpp")
target_compile_features(${_comdare_negativ_compile_tu} PRIVATE cxx_std_23)
target_include_directories(${_comdare_negativ_compile_tu} PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(${_comdare_negativ_compile_tu} PRIVATE Boost::mp11)
comdare_set_platform_defines(${_comdare_negativ_compile_tu})
add_test(NAME ${_comdare_negativ_compile_tu}
         COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target ${_comdare_negativ_compile_tu})
set_tests_properties(${_comdare_negativ_compile_tu} PROPERTIES
    PASS_REGULAR_EXPRESSION "E24-C1-NEGATIV-PROBE"
    RESOURCE_LOCK "comdare_nested_build"
    TIMEOUT 300
    LABELS "goal_v6;phase_d;negativ_compile")

# E-24 C4 (b/5) -- ZWEITE Negativ-Compile-Fixture, nach demselben, in C1 benannten Muster (a)-(d) oben.
# Gegenstand: Bauplan Paragraf 4.1 (Klasse I), Zeile C4 -- eine Dock-Version OHNE HW-Flag muss an der
# ENFORCE-Wache compile-hart brechen. Sie ist der Frueh-Warner fuer Risiko R5 ("ENFORCE-Bruch spaet
# entdeckt"): ohne sie faellt ein flagloses Literal erst im Voll-Bau auf.
# DER RESOURCE_LOCK IST HIER KEINE ZIERDE: er serialisiert diese Fixture gegen die C1-Geschwister --
# zwei gleichzeitige Ninja-Aufrufe auf DEMSELBEN Bau-Verzeichnis wuerden einander blockieren. Genau
# dafuer wurde der Lock in C1 eingefuehrt, und dies ist der erste Fall, der ihn wirklich braucht.
set(_comdare_negativ_compile_tu_c4 test_e24_c4_dock_version_negativ)
add_executable(${_comdare_negativ_compile_tu_c4} EXCLUDE_FROM_ALL "${_comdare_negativ_compile_tu_c4}.cpp")
target_compile_features(${_comdare_negativ_compile_tu_c4} PRIVATE cxx_std_23)
target_include_directories(${_comdare_negativ_compile_tu_c4} PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(${_comdare_negativ_compile_tu_c4} PRIVATE Boost::mp11)
comdare_set_platform_defines(${_comdare_negativ_compile_tu_c4})
add_test(NAME ${_comdare_negativ_compile_tu_c4}
         COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target ${_comdare_negativ_compile_tu_c4})
set_tests_properties(${_comdare_negativ_compile_tu_c4} PROPERTIES
    PASS_REGULAR_EXPRESSION "E24-C4-DOCK-VERSION-NEGATIV"
    RESOURCE_LOCK "comdare_nested_build"
    TIMEOUT 300
    LABELS "goal_v6;phase_d;negativ_compile")

# HY-A1 -- DRITTE Negativ-Compile-Fixture, nach demselben in C1 benannten Muster (a)-(d) oben.
# Gegenstand: das Concept-Gate der Gattung HEURISTIK-ADAPTER. Owner-Auflage 09.08.2026: "die
# Einschraenkung muss HART PER TEST bewiesen werden" -- ein Concept, dessen Ausschluss nie gefahren
# wurde, ist eine Behauptung.
# DREI Diagnosen, drei VERSCHIEDENE Sperren (nicht dreimal dieselbe): S1 Zielfaehigkeit,
# S2 Paar-Konsistenz, plus die CRTP-Wache RerouteGuard an der Konstruktion.
# ZUR MARKER-WAHL, weil hier eine Falle liegt: die Fixture prueft NICHT auf die Diagnose der direkten
# Typbildung. Die bricht zwar korrekt, druckt aber nur "template constraint failure" -- die Meldung des
# zugehoerigen static_assert erscheint NIE, weil der Name danach unbekannt ist. Der Marker kommt daher
# aus je einem Detektor-static_assert, dessen Meldung zuverlaessig gedruckt wird; die direkte Typbildung
# steht daneben als Beweis, dass eine ECHTE Uebersetzung scheitert. Literal nachgestellt vor der Abnahme.
set(_comdare_negativ_compile_tu_hy_a1 test_hy_a1_reroute_gate_negativ)
add_executable(${_comdare_negativ_compile_tu_hy_a1} EXCLUDE_FROM_ALL "${_comdare_negativ_compile_tu_hy_a1}.cpp")
target_compile_features(${_comdare_negativ_compile_tu_hy_a1} PRIVATE cxx_std_23)
target_include_directories(${_comdare_negativ_compile_tu_hy_a1} PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(${_comdare_negativ_compile_tu_hy_a1} PRIVATE Boost::mp11)
comdare_set_platform_defines(${_comdare_negativ_compile_tu_hy_a1})
add_test(NAME ${_comdare_negativ_compile_tu_hy_a1}
         COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target ${_comdare_negativ_compile_tu_hy_a1})
set_tests_properties(${_comdare_negativ_compile_tu_hy_a1} PROPERTIES
    PASS_REGULAR_EXPRESSION "HY-A1-NEGATIV-PROBE"
    RESOURCE_LOCK "comdare_nested_build"
    TIMEOUT 300
    LABELS "goal_v6;phase_d;negativ_compile")

# HY-A1 -- die POSITIV-Seite derselben Gattung: Gate, Partition, Strategie-Struktur, Synthese-Matrix
# und der Weg-C-Pass-through, in gtest. Owner-Auflage: "fuer JEDE Gattung+Genus per google test
# bewiesen durchlaessig" -- ein Gate, das nur ablehnt, waere genauso falsch wie eines, das alles
# zulaesst. Header-only (hybrid/ ist eine reine Header-Stufe, kein add_subdirectory noetig).
comdare_add_test(test_hy_a1_heuristik_adapter_gattung
    SOURCES test_hy_a1_heuristik_adapter_gattung.cpp)
target_include_directories(test_hy_a1_heuristik_adapter_gattung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})

# E-24 C4 (c/5) -- die vier Container-Docks als IPruefDock-Impls, in-process gefahren.
# EIGENER BLOCK statt COMDARE_GOALV6_BOOST_DTESTS (am Bau erzwungen, nicht aus Geschmack): diese TU
# konstruiert/zerstoert AnatomyModuleHandle, und AnatomyModuleHandle::unload() lebt im Loader-.cpp
# (libs/cache_engine/builder/anatomy_module_loader). Der GoalV6-Block linkt nur Boost::mp11 -> der Link
# brach literal mit "undefined reference to ...AnatomyModuleHandle::unload()". Dieselbe Kante zieht schon
# comdare_ce_pruef_dock (libs/.../pruef_dock/CMakeLists.txt) und der SA-Dock-Test oben (:1193-1195).
# Sonst identisch zum GoalV6-Muster: plain int main() (kein gtest) -> add_test direkt, plus Eintrag in
# COMDARE_TEST_TARGETS (#278-Klasse-B-Fix: sonst baut die CI die TU nie -> "Not Run").
set(_comdare_e24_c4_dock_tu test_e24_c4_genus_pruef_docks)
add_executable(${_comdare_e24_c4_dock_tu} "${_comdare_e24_c4_dock_tu}.cpp")
target_compile_features(${_comdare_e24_c4_dock_tu} PRIVATE cxx_std_23)
target_include_directories(${_comdare_e24_c4_dock_tu} PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(${_comdare_e24_c4_dock_tu} PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
comdare_set_platform_defines(${_comdare_e24_c4_dock_tu})
add_test(NAME ${_comdare_e24_c4_dock_tu} COMMAND ${_comdare_e24_c4_dock_tu})
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_comdare_e24_c4_dock_tu})
set_tests_properties(${_comdare_e24_c4_dock_tu} PROPERTIES LABELS "goal_v6;phase_d")

# E-24 C5 (a/5) -- FK-7: die Dock-/Lade-Fehlerpfade werden D2-klassifiziert (Zelle "failed" + Log-Etikett).
# DERSELBE eigene Block wie die C4-Dock-TU darueber und aus DEMSELBEN am Bau erzwungenen Grund: die TU
# konstruiert AnatomyModuleHandle, dessen unload() im Loader-.cpp lebt -- der GoalV6-Sammelblock linkt nur
# Boost::mp11 und braeche mit "undefined reference to ...AnatomyModuleHandle::unload()". Plain int main()
# (kein gtest) -> add_test direkt, plus COMDARE_TEST_TARGETS (#278-Klasse-B-Fix: sonst "Not Run" in der CI).
set(_comdare_e24_c5_fk7_tu test_e24_c5_fk7_dock_fehlerklassen)
add_executable(${_comdare_e24_c5_fk7_tu} "${_comdare_e24_c5_fk7_tu}.cpp")
target_compile_features(${_comdare_e24_c5_fk7_tu} PRIVATE cxx_std_23)
target_include_directories(${_comdare_e24_c5_fk7_tu} PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(${_comdare_e24_c5_fk7_tu} PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader)
comdare_set_platform_defines(${_comdare_e24_c5_fk7_tu})
add_test(NAME ${_comdare_e24_c5_fk7_tu} COMMAND ${_comdare_e24_c5_fk7_tu})
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_comdare_e24_c5_fk7_tu})
set_tests_properties(${_comdare_e24_c5_fk7_tu} PROPERTIES LABELS "goal_v6;phase_d")

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
    # A8-S1 (2026-08-04): DAUERHAFTE Wache gegen den stillen T17-Messwert-Verlust (Befund B-6). Prueft die
    # Mess-Kette (Pfad-A-POD, Pfad-B-POD, CSV-Naht, Knoten-Projektion) IMMER gegen kV3AxisCount, nie gegen ein
    # Literal -> bricht bei kuenftigem Achsen-Zuwachs, statt den neuen Slot still zu verlieren.
    test_a8s1_t17_vollzaehligkeit     # A8-S1 -- T17-Vollzaehligkeit der Mess-Kette gegen kV3AxisCount
    # A8-S3 (2026-08-04): DAUERHAFTE Wache gegen den strukturell-0-Befund der T17-OBSERVER-Zeile. A8-S1 heilte
    # die Kopier-GRENZE (seg_ns), diese TU den LESE-ZEITPUNKT (axis_stats): sie verlangt Zaehler UND Zeit
    # derselben Op-Schleife zugleich und trennt strukturell-0 von ehrlich-0 (Disk- vs. MemoryOnlyTarget).
    test_a8s3_t17_stats_lesezeitpunkt # A8-S3 -- T17-Observer-Lesezeitpunkt (axis_stats nach dem Treiben)
    # A8-S5 (2026-08-04) Familie 04_execution = PILOT des GATE-MUSTERS fuer alle S5-Familien. Die
    # Konformitaets-Wache pinnt auf TYP-EBENE gegen die Achsen-REGISTRIES (nie gegen Literale/Dateilisten),
    # dass kein Familien-Organ Speicher an der Allokator-Achse vorbei fuehrt; das wiederverwendbare
    # Praedikat liegt in s5_family_alloc_conformance.hpp (Folge-Familien tauschen NUR die Typ-Liste).
    test_s5_04_execution_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 04_execution
    # Zweite Haelfte desselben Gate-Musters: LAUFZEIT-Beleg, dass der Scrub die Familie nicht stumm
    # geschaltet hat. Verlangt Zeit UND Zaehler derselben Op-Schleife zugleich (Anti-Phantom) plus den
    # ehrlichen Nullpunkt der None-Strategie; Achsen-Indizes aus kCompositionAxisNames abgeleitet.
    test_s5_04_execution_perf_sanity  # A8-S5 -- Perf-Sanity Familie 04_execution (keine 0-/Phantom-Zeiten)
    # A8-S5 (2026-08-04) Familie 03_placement = die ERSTE Familie, die Form B (Speicher ueber die Allokator-
    # Achse) wirklich benutzt. Ihre Konformitaets-Wache traegt deshalb ZUSAETZLICH zum Typ-Pin den LAUFZEIT-
    # Beleg der realen Verdrahtung und des R1-Memento-Rebinds (Form-B-Grenze, s5_family_alloc_conformance.hpp:31).
    test_s5_03_placement_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 03_placement
    # Zweite Haelfte desselben Gate-Musters fuer 03: Zeit UND Zaehler derselben Op-Schleife (Anti-Phantom)
    # plus ehrliche Nullpunkte + Strategie-Kontrast; Achsen-Indizes aus der Namensliste abgeleitet, nie Literal.
    test_s5_03_placement_perf_sanity  # A8-S5 -- Perf-Sanity Familie 03_placement (keine 0-/Phantom-Zeiten)
    # A8-S5 (2026-08-04) Familie 01d cache_traversal = ERSTE Folge-Familie nach dem Pilot und zugleich der
    # Muster-Beweis der 01c-Mechanik im Kleinen: dieselbe Nicht-Template-Klassen-Form wie die 17 Top-Level-
    # Algos der Familie 01c. Die Familie faehrt Schnitt-Form (B) (Allokator-Achse) statt (A) (heap-frei),
    # weil register_entry/rehash unbeschraenkt wachsen -- deshalb belegt sie den von Form B geforderten
    # realen Verdrahtungs-Anker zusaetzlich compile-hart (Konformitaets-TU) UND zur Laufzeit (Perf-Sanity).
    test_s5_01d_traversal_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 01d cache_traversal
    # Perf-Sanity derselben Familie: Zeit UND Zaehler derselben Op-Schleife (kV3AxisSchema[T1]: [3] register
    # als allozierende, [0] resolve als lesende Op) + ehrlicher Nullpunkt OHNE None-Strategie (unregister
    # bleibt 0; hit+miss == resolve mit beiden Seiten real) + Rehash-Pfad ueber die Achse.
    test_s5_01d_traversal_perf_sanity # A8-S5 -- Perf-Sanity Familie 01d (keine 0-/Phantom-Zeiten)
    # A8-S5 (2026-08-04) Familie 02_layout / Sub-Scheibe 02a (node_type + memory_layout + path_compression +
    # serialization; filter/SuRF = 02b, bewusst NICHT hier). Diese Familie traegt den HERZ-Befund der Welle:
    # die node-Chunk-Stores DEKLARIERTEN ihren allocator_type schon vor dem Scrub (A ist Template-Parameter),
    # haetten den Form-B-Zweig also bereits bestanden -- und trotzdem lief der Chunk-INDEX am Default-Allokator.
    # Die Konformitaets-Wache pinnt deshalb zusaetzlich zur Typ-Ebene die REALE Verdrahtung mit einer Aussage,
    # die am Alt-Stand FAELLT (allocation_count ECHT > chunk_alloc_count), plus den R1-Memento-Rebind.
    test_s5_02a_layout_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 02a (inkl. HERZ-Beleg)
    # Zweite Haelfte desselben Gate-Musters fuer 02a: Zeit UND Zaehler derselben Op-Schleife (Anti-Phantom)
    # plus ehrliche Nullpunkte (path_compression_none haelt weiterhin KEINEN Trie) und der direkte Blick in die
    # geSCRUBbten Strukturen; Achsen-Indizes aus der Namensliste abgeleitet, nie Literal.
    test_s5_02a_layout_perf_sanity    # A8-S5 -- Perf-Sanity Familie 02a (keine 0-/Phantom-Zeiten)
    # A8-S5 (2026-08-04) Familie 01a Pool-/Layout-Stores = die dichteste Form-B-Flaeche des Scrubs. Ihre
    # Typ-Liste wird NICHT gepflegt, sondern ABGELEITET (AllStrategies -> organ_for_search_algo_t -> das
    # zweite Template-Argument des Organs = der Store); die einzige Handnennung ist das DEFERRED Masstree-
    # Substrat, das keinen Registry-Wrapper hat. Zwei Mitglieder tragen einen Container IM Element
    # (Skip-Listen-Forward-Turm, START-disc/kids) -- der COW-Block ist der Detektor gegen die stille
    # Cross-Strategy-Allokation, die eine gewoehnliche Vektor-Kopie dort erzeugt haette.
    test_s5_01a_pool_stores_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 01a
    # Perf-Sanity derselben Familie: Zeit UND Zaehler derselben Op-Schleife ueber DREI verschiedene
    # Substrat-Anatomien (ART / SkipList / SwissTable), plus KOHAERENZ-Probe gegen die direkt am Organ
    # gelesene Achsen-Statistik (die T6-Zeile muss aus DIESEM Store stammen) und den ehrlichen Nullpunkt
    # des ungetriebenen Organs. Achsen-Indizes aus kCompositionAxisNames abgeleitet, nie Literal.
    test_s5_01a_pool_stores_perf_sanity # A8-S5 -- Perf-Sanity Familie 01a (keine 0-/Phantom-Zeiten)
    # A8-S5 (2026-08-04) Familie 02_layout / Sub-Scheibe 02b (filter/SuRF) = die staerkste vendored-ext-Naehe
    # der S5-Strecke: die composable-Organe sind eine original-getreue Portierung aus ext/traversal/P10-SuRF
    # MIT Paper-Zeilen-Ankern. Deshalb traegt die Konformitaets-Wache hier eine DRITTE Aussage neben Typ-Pin
    # und Verdrahtungs-Beleg: den PAPERTREUE-Beleg (no-false-negative + Kreuzbeleg gegen die exakte
    # S1-Ground-Truth) -- Papertreue IST hier die Messaussage. Zusaetzlich der VERSCHACHTELUNGS-Beleg beider
    # Ebenen des LOUDS-Level-Index (uses-allocator-Konstruktion greift ohne scoped_allocator_adaptor NICHT).
    test_s5_02b_filter_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 02b (inkl. Papertreue)
    # Zweite Haelfte desselben Gate-Musters fuer 02b. BESONDERHEIT (im TU-Kopf ausgeschrieben): die
    # KOMPOSITIONS-filter-Achse (Bloom/Cuckoo/RangeSurf/Xor) liegt im T14-Pfad und wurde NICHT geSCRUBbt;
    # die geSCRUBbten composable-Organe liegen NICHT im T14-Pfad. Die Wache prueft deshalb BEIDES getrennt:
    # T14-Spalten (probe/pos/neg/hash_probes, Schema-Zeile gepinnt) fuer die Achsen-Messfaehigkeit UND das
    # Anti-Phantom-Paar direkt am Scrub-Gegenstand (Zeit + Achsen-Allokations-Zaehler derselben Op-Schleife).
    test_s5_02b_filter_perf_sanity    # A8-S5 -- Perf-Sanity Familie 02b (keine 0-/Phantom-Zeiten)
    # A8-S5 Sub-Scheibe 01b (composable-REST): nicht die Substrate, sondern die Organe darueber, ihre
    # Huellen und die Schalen-Puffer (Memento, Scan-Zwischenablage, Pilot-Store RawSlotStore<>). Die TU
    # pinnt BEIDE Schnitt-Formen JE ORGAN: Form A heap-frei mit typ-abgeleiteter CT-Kappe fuer die drei
    # Trie-Familien (Tiefe strukturell durch die Schluesselbreite beschraenkt), Form B ueber die Achse
    # fuer BST/B-Baum (Tiefe datenabhaengig -- eine CT-Kappe waere dort keine staerkere Aussage, sondern
    # eine falsche). Ebenen 2/3 waren in 01a noch reiner AUSWEIS; hier werden sie zur Aussage und deshalb
    # auch belegt (Walk-Delta VOR jedem Restore, Memento-/Scratch-Zaehler, ehrliche Nullpunkte).
    test_s5_01b_composable_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 01b
    # Perf-Sanity derselben Familie. Schaerfer als 01a, weil 01b den WALK-ALGORITHMUS dreier Trie-Familien
    # umgestellt hat: ein still verstummender Walk wuerde die DEG-1-Key-Ernte des Mess-Pfads leeren, ohne
    # dass ein Test rot wird. Zeit UND Zaehler derselben Schleife, plus die Form-Unterscheidung (Form B
    # MUSS beim Walk ueber die Achse allozieren, Form A darf es NICHT) und zwei ehrliche Nullpunkte.
    test_s5_01b_composable_perf_sanity # A8-S5 -- Perf-Sanity Familie 01b (keine 0-/Phantom-Zeiten)
    # HAERTUNG Posten 64 (2026-08-04): der FEHLSCHLAG-VERTRAG des StdAllocatorAdapter, negativ belegt.
    # Nach dem A8-S5-Schnitt fuehren mapping/value_handle/cache_traversal + die vier Pool-Stores ihren
    # Speicher ueber die Allokator-Achse; die meldet OOM per nullptr. Der Adapter uebersetzt das jetzt an
    # EINER Stelle in std::bad_alloc -- sonst konstruiert der besitzende Container in Nullspeicher (UB).
    # Die TU beweist den Wurf an einer Stub-Strategie, die IMMER erschoepft ist (echtes OOM ist nicht
    # deterministisch erzeugbar), plus Zaehler-Ehrlichkeit, Zero-Size-Neutralitaet und die
    # Positiv-Kontrolle an der realen Achsen-Default-Strategie.
    # A8-S5 (2026-08-05) Familie 01c = die REGISTRY-ORGANE der Achse 03a, PILOT-Stand (linear_scan).
    # Anders als 01a/01b/01d bindet diese Familie nicht an einen benannten Achsen-DEFAULT, sondern an die
    # T6-Wahl der KOMPOSITION (Owner-KERN "Option B strikt"). Das geht nur ueber den Zwei-Ebenen-Schnitt:
    # namens-stabile Fassade (Registry-Identitaet, type_name reist in die XML) ueber parametriertem
    # CRTP-Core, plus Rebound-Leaf fuer die fremde Strategie. Die Wache pinnt deshalb BEIDE Ebenen UND
    # ihre Nicht-Vermischung, den Level-0-is_same-Pin (der ganze golden-Beweis der Scheibe), die
    # name()-Invarianz (T6-Wahl darf nie in den serialize-Schluessel lecken) und den Kontrast-Beweis nach
    # 02a-HERZ-Vorbild (drei Groessen gegeneinander statt eines geschenkten ">0").
    test_s5_01c_fassaden_conformance # A8-S5 -- Fassaden-/Durchbindungs-Konformitaet Familie 01c
    # A8-S5 (2026-08-05) Familie 05_write_path_io / RESTSTRECKE queuing = die vom S5-Abschnitts-Truth-Check
    # gefundene VOLLSTAENDIGKEITS-LUECKE (wf_94ca9f27 R1, Auflage 2+3). Der bisherige 05-Nachweis war ein
    # grep ueber axes/io_dispatch/ + axes/persistence_target/ und meldete "leer" -- korrekt, aber gegen ein
    # zu kleines UNIVERSUM: kOrganGruppe05 hat VIER Achsen, und queuing_q1/q2 haben ueberhaupt keinen
    # axes/-Ordner (sie leben nur in der topics/-Doppelwurzel). Diese TU macht die stille Annahme
    # unmoeglich: das Universum kommt aus der drift-bewachten EINZIGQUELLE (lager_baum_writer.hpp
    # kOrganGruppe05), die Deckung wird BEIDSEITIG gepinnt (jede Achse gedeckt UND jede Deckung in der
    # Familie), und die Quell-WURZEL wird nicht angenommen, sondern von den Organen selbst erfragt
    # (COMDARE_DEFINE_ORGAN_LOCATION -> header_include). Dazu der Form-Ausweis je Organ, der
    # Kontrast-Pin (Q1 darf NICHT heap-frei sein -- sonst traegt die Wache nichts ueber Form B), der
    # LAUFZEIT-Verdrahtungs-Beleg je Q1-Puffer (Achsen-Zaehler > 0 nach echten put(); am Alt-Stand
    # strukturell 0) und der COW-/Memento-Rebind-Beleg am kopierten Organ.
    test_s5_05q_queuing_alloc_conformance # A8-S5 -- Allokator-Achsen-Konformitaet Familie 05 (4/4 Achsen)
    # Zweite Haelfte des Gate-Musters: die Konformitaets-TU pinnt auf TYP-Ebene, diese hier zur LAUFZEIT,
    # dass der Q1-Schnitt (kompletter Wechsel des Speicher-Unterbaus auf die Allokator-Achse) die Achse
    # nicht still stumm geschaltet hat. Zeit UND Zaehler DERSELBEN Op-Schleife, Feld-Indizes gegen
    # kV3AxisSchema gepinnt, plus der ehrliche Nullpunkt BEIDER Achsen (NoBuffer peak_size==0 bei
    # laufendem Betrieb an anderer Spalte; LazyFlush/EagerFlush tauschen full_flush/no_flush).
    test_s5_05q_queuing_perf_sanity # A8-S5 -- Perf-Sanity T15/T16 Familie 05
    test_h64_allocator_adapter_failure_contract # Posten 64 -- StdAllocatorAdapter: nullptr -> std::bad_alloc
    # POSTEN 80 (2026-08-05, A8-S5 01c Vorlauf 0): der 01b-Vorbehalt "-O3/MimallocAllocator/'jmp .'" an
    # seiner ECHTEN Ursache gepinnt -- die drei Statistik-Weiterleiter der axis_06-CRTP-Basis rufen sich
    # selbst auf, wenn die Strategie keinen eigenen Member deklariert (bis 05.08. nur ExgenAllocator).
    # Kein Compiler-Bug, kein UB im Puffer: eine fehlende Deklaration, die -O3 als Endlosschleife und -O1
    # als Selbstaufruf uebersetzt und die ASan/UBSan nicht sehen koennen. Die TU pinnt den Flaechen-Beweis
    # ueber die ACHSEN-REGISTRY (nicht ueber eine Namensliste), den Laufzeit-Beweis am Original-Gegenstand
    # (AxisBoundBuffer-Memento ueber mimalloc) und die GEGENPROBE (Stub ohne Member wird erkannt).
    test_h81_allocator_statistics_forwarder_recursion # Posten 80 -- CRTP-Weiterleiter: Selbst-Rekursion
    # A1-SCHEIBE "Wurf-Vertrag" (2026-08-06, Owner-Posten 71/72/73/74 + Pool-Konventions-Ausreisser):
    # die Fortsetzung von Posten 64 an den Stellen, die dessen EINE Uebersetzung nicht erreichte. Die TU
    # belegt alle vier Zugriffswege der Achse gegen DENSELBEN Fehlschlag-Vertrag: pmr-Adapter (Posten 71),
    # Ueberlauf-Wache des std-Adapters (Posten 72), roher Store-Konsum ueber allocate_or_throw (Posten 74)
    # und den einen Strategie-Ausreisser (Pool warf, statt nullptr zu melden). Der Leck-Beweis des
    # Kopier-Ktors laeuft als BILANZ ueber eine zaehlende Achsen-Variante, nicht als Behauptung.
    test_a1_wurf_vertrag_allokator_store # A1 -- EIN Wurf-Vertrag ueber Allokator-/Store-Stellen
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

# A8-S5 01c: die Fassaden-Wache liest die COMMITTETE Registry-XML (Single-Source per Define, NICHT
# kopiert -- Muster der uebrigen Registry-Wachen). Sie beweist die zweite Haelfte des Byte-Arguments:
# der Roundtrip zeigt "XML == Reflektion des Codes", diese Zeile zeigt, dass die reflektierte Form die
# ARGLOSE Fassaden-Form ist (ein Template-Kopf am Wrapper braechte Template-Argumente in type=).
# Die XML wird NUR GELESEN (TABU-Wache).
target_compile_definitions(test_s5_01c_fassaden_conformance PRIVATE
    "COMDARE_CE_AXIS_REGISTRY_XML=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml\"")

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
    test_genus_organ_binding        # INC-1b: Organ-Achsen->Gattungs-Bindung (typisiert + drift-guarded)
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
    test_kf16b_build_resilience
    test_w6_parallel_provision      # W6 (§32-F7): paralleler provision-Bau-Pool + positions-treue Sink-Ordnung
    test_w11_async_push_pump        # W11 (§43.c): BAU-Modus async Push-Pump + ArtifactCache-Teil-Marker (Fake-mc)
    test_s5_artifact_cache_bounded  # S5-Rest: measure-drop-PUT (curl) BOUNDED gegen Blackhole (Timeout-Wache)
    test_s1_cache_key_prefix        # S1 (#46a): cache_key_prefix-Segment-Montage (+ceb/+mtool/+mrg), Push-Reihenfolge
    # E-24 C10 (c) / G6: die Identitaets-Bilanz des Major-Bumps -- +ceb= BEWEGT, Stempel-Preimage und
    # CRC64-Anker UNBEWEGT. Liegt hier, weil die TU dieselben drei Flaechen anfasst wie test_s1
    # (artifact_cache-Key-Naht) und dieselbe Include-Menge braucht (source_catalog + generated-Flags).
    test_e24_c10_g6_identitaets_bilanz
    test_s2_pull_tier_binary        # S2 (#46a): pull_tier_binary + pull_tier_prefix (Fake-mc: Hit/Miss/Halb/Prefix/inert)
    test_45_parallel_measure_loop   # #45 (§16.2-M1/§61-MODI): collect_ordered + resolve_measure_parallelism + parse-Naht
    test_cache_mc_timeout           # Cache-Resthygiene: mc-Timeout-Wrapper (build_mc_argv literal + End-to-End-Hang-Cap)
    test_r2_suffix_wachen           # Lane F R2 (O-8 Schritt 11): T-a/T-b/T-c auf der Suffix-Single-Source
    test_w10_c4_zellwert_naht       # W10-C4: Zellwert-Wertform/Define-Argument, Diskriminierung, na-fail-closed,
                                    #         W10-M2 (Contract-Minor 2, +ceb= am Perm-Pfad, Store-Key-Dedupe)
    test_e24_c9_fk5_fehlerraum        # E-24 C9 / FK-5: Organ-Achsen-Fehlerraum (Cross-Layer-Wahrheitsquellen-Naht,
                                       #         18-Achsen-Vollstaendigkeit, Negativ-Probe, D1/D2-Domaenen-Trennung)
    test_a15_fk3_axis_error_traits    # A15 / FK-3: Achsen-Fehlerraeume (20 Familien, Boost-Hermetik-Wache mit
                                       #         eigener Nicht-Blind-Probe, 4 Invarianten-Bissproben, Laufzeit-Biss
                                       #         an den 3 Mess-Achsen)
    test_a15_fk4_organ_slot_traits    # A15 / FK-4: Organ-SLOT-Fehlerraeume T0..T17 (Namen gegen
                                       #         kCompositionAxisNames, 18 gegen abi::kOrganAxisCount, D2-Saetze
                                       #         deckungsgleich zu FK-5 = Anti-Parallelstruktur-Wache)
    test_fk6_algo_error_classes       # FK-6: ALGORITHMEN-Fehlerraeume (die per-Varianten-Verfeinerung, die
                                       #         FK-5 als "deklarierte Luecke" offen liess). BEDINGT statt
                                       #         flaechig: gemessen tragen 26 von 126 registrierten Varianten
                                       #         requires_specialized_hardware(), GENAU 1 meldet true
                                       #         (pim_malloc -> quelle_nicht_verfuegbar). Koeder-Proben +
                                       #         Uebergangsliste (Laenge 0, darf nur schrumpfen)
    test_resthygiene2_organ_fingerprint) # Resthygiene-2: Organ-Fingerprint sort+concat == find|LC_ALL=C sort|xargs cat
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
    test_d_v42_abi_telemetry_coupling
    # A8-S4 (2026-08-04): Konstitutiv-Ketten-Wache zur Konstitutiv-Matrix (Gattungs-Vertrag, Dossier 3.3).
    # MESSUNG-AN-Seite des Paares; die Release-Seite ist test_a8s4_release_pfad_neutralitaet (eigener Block
    # weiter unten, weil sie die Mess-Defines per #undef ABSCHALTET und damit NICHT in diese Liste gehoert).
    test_a8s4_konstitutiv_kette)
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

# A8-S4 (2026-08-04) -- RELEASE-PFAD-NEUTRALITAETS-BEWEIS. Eigener Block, weil diese TU als EINZIGE
# bewusst OHNE Messung uebersetzt: sie schaltet COMDARE_MEASUREMENT_ON/CE_ENABLE_STATISTICS/
# EXPERIMENT_MODE_ON per #undef in der ERSTEN Quellzeile ab (portabel, kein -U/-D-Kunstgriff) und
# beweist damit am ECHTEN Adapter, dass die beobachtenden Kopplungen im Release-Pfad nicht existieren.
# Bewusst NUR Boost::mp11 (header-only) verlinkt: kein Objekt aus einem messungs-AN uebersetzten
# comdare-Target, damit die Release-Uebersetzungseinheit fuer sich allein steht.
# COMDARE_A8S4_ABI_ADAPTER_SRC = Quellpfad fuer die Praeprozessor-Pfad-Wache (NUR gelesen).
add_executable(test_a8s4_release_pfad_neutralitaet "test_a8s4_release_pfad_neutralitaet.cpp")
target_compile_features(test_a8s4_release_pfad_neutralitaet PRIVATE cxx_std_23)
target_include_directories(test_a8s4_release_pfad_neutralitaet PRIVATE ${COMDARE_MCE24_INCLUDES})
target_link_libraries(test_a8s4_release_pfad_neutralitaet PRIVATE Boost::mp11)
comdare_set_platform_defines(test_a8s4_release_pfad_neutralitaet)
target_compile_definitions(test_a8s4_release_pfad_neutralitaet PRIVATE
    "COMDARE_A8S4_ABI_ADAPTER_SRC=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/anatomy/abi_adapter.hpp\"")
add_test(NAME test_a8s4_release_pfad_neutralitaet COMMAND test_a8s4_release_pfad_neutralitaet)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_a8s4_release_pfad_neutralitaet)
set_tests_properties(test_a8s4_release_pfad_neutralitaet
                     PROPERTIES LABELS "g4;muster_f;standalone;release_mode" TIMEOUT 120)

# Bau-INC-2c.opt Naht #2 (Option A, Verifikations-Test-Teil): der Layout-Sensitivitaets-Check in
# test_all19_segment_timer (seg_memory_layout_ns aos_strict-Stride-48 vs cache_line_aligned-Stride-64) ist NUR
# UNTER Optimierung >5% sichtbar — im unoptimierten Default-Build maskiert der Instrumentierungs-Overhead den
# Cache-Line-Effekt (Median-of-7 misst physikalisch ~3-4% < Schwelle → CI-Rot). -O3 (deterministisch, IEEE-754-treu,
# NICHT Ofast) macht den Effekt real messbar. Reflektiert die opt_level=O3-Ausprägung der Compiler-System-Achse
# (optimization_level_sub_axis.hpp); der volle XML-permutierte opt_level-Raum je mess-charakteristischem Target
# folgt mit dem Planer (2c.opt-g/h). Nur dieses eine Target (die Observer-Tests der Liste tragen honest-0-Kontrakte,
# die unter -O3 nicht ohne Not verschoben werden sollen).
comdare_apply_optimization_level_flags(test_all19_segment_timer O3)

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
# A8-S5-01b: composable_search.hpp bindet den Slot-Speicher an die Allokator-Achse (axis_06) und zieht damit
# denselben Include-Satz wie jeder andere organ-komponierende Test (src/measurement fuer MeasurableObserver) --
# Praezedenz test_188_4bb0/test_v41_topic_traversal. Der Test bleibt standalone (kein gtest, kein Codegen).
target_include_directories(test_conformance_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src")
# A8-S5-01b-Hotfix (05.08.): die neue axis_06-Include-Kette zieht src/concepts/legacy_original_code_
# strategy_concept.hpp (boost::mp11) -- ohne das vendored Target kompiliert die TU nur dort, wo ein
# System-Boost existiert (falsches-Gruen-Klasse 'lokale Voll-Bau-Luecken'; CI-Job contract:conformance
# mit frischem Configure fing es). Include-Pfad kommt ueber das Interface-Target, kein System-Boost.
target_link_libraries(test_conformance_gate PRIVATE Boost::mp11)
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

# A8-S5 PHASE B (2026-08-05) -- KONTRAST-BEWEIS der Treiber-Durchbindung (EffectiveSearchAlgo).
# Instanziiert SearchAlgorithmAbiAdapter mit einer Komposition, deren Allokator-Achse eine TYP-GLOBAL
# zaehlende Test-Strategie traegt, und faehrt alle DREI Mess-Treiber. Die Aussage ist eine exakte
# Gleichung (Treiber-Eigenanteil + gemessener Organ-Anteil) plus der Fassaden-Gegenpol (Bewegung 0) --
# ein blosses ">0" waere schon am ALT-Stand gruen gewesen, weil der Treiber selbst ueber dieselbe
# Strategie alloziert. COMDARE_MEASUREMENT_ON=1 ZWINGEND: die drei Treiber existieren nur bei
# Messung-AN. Boost::mp11 explizit (die Registry reist ueber den Umbrella mit -- Lehre cda964e0:
# lokal gruen beweist gegen fehlende vendored-Links NICHTS). Standalone (plain int main, kein gtest).
add_executable(test_phase_b_effective_search_algo_contrast "test_phase_b_effective_search_algo_contrast.cpp")
target_compile_features(test_phase_b_effective_search_algo_contrast PRIVATE cxx_std_23)
target_include_directories(test_phase_b_effective_search_algo_contrast PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_phase_b_effective_search_algo_contrast PRIVATE Boost::mp11)
comdare_set_platform_defines(test_phase_b_effective_search_algo_contrast)
target_compile_definitions(test_phase_b_effective_search_algo_contrast PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_phase_b_effective_search_algo_contrast COMMAND test_phase_b_effective_search_algo_contrast)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_phase_b_effective_search_algo_contrast)
set_tests_properties(test_phase_b_effective_search_algo_contrast PROPERTIES LABELS "contract;standalone;a8;alloc")

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

# Bau-INC-1a (Q1-Ruling): Achsen-Dach-Guard — topics::Axis<Derived>-Dach + CebSystemAxis/SystemAxis-
# Geschwister-Wurzeln bleiben zustandslos, vtable-frei und familien-getrennt (Blut-Direktive). Additiv,
# golden/ABI-neutral (reiner static_assert-Test).
comdare_add_test(test_striktheit_axis_dach_guard
    SOURCES test_striktheit_axis_dach_guard.cpp)
target_include_directories(test_striktheit_axis_dach_guard PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_compile_definitions(test_striktheit_axis_dach_guard PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# Paket A14 / OS-U1 (Owner-Entscheid E3, 02.08.2026): Wachen-Test der DREI operating_system-Unter-Achsen
# (os_version/kernel/build). Er ist zugleich die TU, in der operating_system_sub_axes.hpp im Default-Sweep
# tatsaechlich uebersetzt wird -- ein nie inkludierter Header haette nur dekorative Wachen. Additiv,
# golden/ABI-neutral; die Stempel-Neutralitaet (A-15/K5) wird als Gegenprobe mitgeprueft.
comdare_add_test(test_a14_os_sub_axes_guard
    SOURCES test_a14_os_sub_axes_guard.cpp)
target_include_directories(test_a14_os_sub_axes_guard PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_a14_os_sub_axes_guard PRIVATE Boost::mp11)
target_compile_definitions(test_a14_os_sub_axes_guard PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# Lane C / Paket C-5 -- NACHREGISTRIERUNG A13-M2 (Review-BEFUND-1, Owner-Entscheid F-WAISEN Variante (b)
# vom 02.08.2026). Die TU lag seit 26.07. als WAISE im Baum (nie in dieser Datei eingetragen, damals wegen
# Lane-A-Konflikt-Zone). Genau diese Luecke war falsches Gruen: die A13-M2-Verschaerfung von
# SystemMetaMetaAxisConcept (axis_code_version-Pflicht) brach die TU compile-hart, und der K1-Realm-Umzug
# von load_framework hatte eine ihrer Erwartungen still superseded -- beides blieb im Voll-ctest unsichtbar.
# Sie ist die einzige der acht Waisen-TUs, die von A13-M2 beruehrte Header inkludiert; die uebrigen sieben
# bleiben bewusst unangetastet (Abschluss-Aufraeumpass-Kandidaten).
# Include-Menge MINIMAL am Ist geprueft (g++ -fsyntax-only): der Test zieht ausschliesslich
# cache_engine/measurement/*-Header, also genuegt EIN Include-Dir -- kein builder/, kein common/, keine
# generierten Achsen-Dirs. Boost::mp11 ist echte Abhaengigkeit (meta_meta_identity.hpp/meta_meta_admission.hpp).
comdare_add_test(test_meta_meta_halbordnung
    SOURCES test_meta_meta_halbordnung.cpp)
target_include_directories(test_meta_meta_halbordnung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_link_libraries(test_meta_meta_halbordnung PRIVATE Boost::mp11)
target_compile_definitions(test_meta_meta_halbordnung PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# Fehlerklassifizierungs-Framework INC-29.0 (Task #29): reiner static_assert-Taxonomie-Guard (axis_error.hpp,
# header-only Enums; kein boost, keine lib). Beweist D1/D2-Taxonomie + Count-Drift-Guards + CSV-Token-Vokabeln.
comdare_add_test(test_axis_error_taxonomy
    SOURCES test_axis_error_taxonomy.cpp)
target_include_directories(test_axis_error_taxonomy PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

# Fehlerklassen-Framework A15/FK-0 (Task #29, Owner-Entscheid E6 vom 02.08.: "Fehlerklassen bitte jetzt
# mit bauen"): der RF-3-Guard (§70.3) zur fuenften D1-Klasse BetriebssystemFeatureFehlt wird hier am Join
# nachregistriert (bis 02.08. lag er unregistriert im Baum, Sammel-Registrierung war so vorgesehen).
# Er beweist ZWEI Dinge, die der Nachbar test_axis_error_taxonomy NICHT prueft: die PAARWEISE
# Distinktheit aller fuenf D1-Etiketten und die Anhaenge-Wache (hinter dem Count darf kein etikettierter
# Enum-Wert liegen). Include-Menge deshalb identisch zum Nachbarn: der Test zieht ausschliesslich
# measurement/axis_error.hpp (header-only Enums, kein boost, keine lib).
# Bewusst OHNE gtest geschrieben (eigenes main, Exit 0/1) -- comdare_add_test linkt gtest/gtest_main nur
# als Archive; der main-tragende Archiv-Member wird nicht gezogen, weil main hier schon definiert ist.
comdare_add_test(test_rf3_betriebssystem_feature_fehlt
    SOURCES test_rf3_betriebssystem_feature_fehlt.cpp)
target_include_directories(test_rf3_betriebssystem_feature_fehlt PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

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
# Lager-TP1-Nachbesserung N3 (B-2/B-3): Iterator-Ebene-Anker fuer den Bau-Filter -- builds-Schrumpfung,
# Skip-Buchung, Fenster-Reservierung, done-Menge des Paragraf-38-Kanals und bestand_lager_skips gegen
# den ECHTEN run_planer_driven_provision/run_lazy_static_then_dynamic (Stub-Compile, FakeTransport,
# kein minio). Include-/Define-Satz wie test_lazy_resume_binary (schwerer Host-Treiber-Header).
add_executable(test_tp1_planer_filter_iterator "test_tp1_planer_filter_iterator.cpp")
target_compile_features(test_tp1_planer_filter_iterator PRIVATE cxx_std_23)
target_include_directories(test_tp1_planer_filter_iterator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
# Der Test RUFT run_lazy_static_then_dynamic wirklich auf (anders als test_lazy_resume_binary) --
# der Linker braucht deshalb die beiden Nicht-Header-Bausteine des Laufs (WorkloadGenerator,
# AnatomyModuleLoader), auch wenn der provision_only-Zweig sie nie betritt.
target_link_libraries(test_tp1_planer_filter_iterator PRIVATE Boost::mp11
    comdare::anatomy_module_loader comdare::workload_driver)
comdare_set_platform_defines(test_tp1_planer_filter_iterator)
target_compile_definitions(test_tp1_planer_filter_iterator PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_tp1_planer_filter_iterator COMMAND test_tp1_planer_filter_iterator)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_tp1_planer_filter_iterator)
set_tests_properties(test_tp1_planer_filter_iterator PROPERTIES LABELS "bestandslog;bau_filter;standalone")

# A5 / F5-ETA-Paket -- der WIRK-Nachweis am echten run_planer_driven_provision: traegt die Reservierung
# ihre eta_s WAEHREND der Slice laeuft (Beweis-Anker der F5-Spez), bleibt sie bei zu kleinem Fenster
# EHRLICH leer, und ueberlebt der zuvor registrierte Completion-Hook die zweite Registrierung.
# Der Store fuehrt eine HISTORIE -- der Endzustand allein koennte die Frage nicht beantworten.
# Include-/Link-/Define-Satz identisch zu test_tp1_planer_filter_iterator (derselbe schwere Header).
add_executable(test_a5_eta_slice_verdrahtung "test_a5_eta_slice_verdrahtung.cpp")
target_compile_features(test_a5_eta_slice_verdrahtung PRIVATE cxx_std_23)
target_include_directories(test_a5_eta_slice_verdrahtung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_a5_eta_slice_verdrahtung PRIVATE Boost::mp11
    comdare::anatomy_module_loader comdare::workload_driver)
comdare_set_platform_defines(test_a5_eta_slice_verdrahtung)
target_compile_definitions(test_a5_eta_slice_verdrahtung PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_a5_eta_slice_verdrahtung COMMAND test_a5_eta_slice_verdrahtung)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_a5_eta_slice_verdrahtung)
set_tests_properties(test_a5_eta_slice_verdrahtung PROPERTIES LABELS "bestandslog;eta;standalone")

# T2-A/F4 (Fable-Review MITTEL-1, 2026-08-06) -- die DURCHREICHUNG der Plan-Ablage durch die
# run_profile-Fassade. test_tp1_planer_filter_iterator (Fall 11) belegt die MECHANIK auf Iterator-Ebene,
# setzt cfg.batch_plan_datei dabei aber selbst -- und sprang damit ueber genau das Glied hinweg, das im
# produktiven Weg fehlte. Dieses Ziel ruft das ECHTE run_profile (Stub-Compile, FakeTransport, kein minio)
# und prueft die Kette RunProfileArgs -> make_cfg -> LazyRunConfig an ihrer Wirkung (Plan/Zaehler auf der
# Platte, Resume im Folgelauf, Abwesenheit bei leerem Pfad).
# Include-/Link-Satz = test_tp1_planer_filter_iterator (echter run_lazy-Aufruf) PLUS der katalog-schwere
# Satz von test_axis_sweep_coverage (profile_run_entry.hpp zieht source_catalog/sota_catalog) und der
# XML-Parser fuer load_thesis_profile.
add_executable(test_t2a_f4_facade_plan_durchreichung "test_t2a_f4_facade_plan_durchreichung.cpp")
target_compile_features(test_t2a_f4_facade_plan_durchreichung PRIVATE cxx_std_23)
target_include_directories(test_t2a_f4_facade_plan_durchreichung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_t2a_f4_facade_plan_durchreichung PRIVATE Boost::mp11
    comdare::anatomy_module_loader comdare::workload_driver comdare_builder_xml_config_parser
    # A9-S5: profile_run_entry.hpp haengt jetzt die Ergebnis-Mappe ein. ErgebnisMappenFactory::oeffne()
    # ist inline im Header und ODR-nutzt detail::erzeuge_xlsx_ergebnis_mappe() in JEDEM Zweig -- wer den
    # Entry-Header inkludiert, linkt deshalb comdare::lager_ablage (gleiche Lage wie die a9s3-Tests).
    comdare::lager_ablage)
comdare_set_platform_defines(test_t2a_f4_facade_plan_durchreichung)
target_compile_definitions(test_t2a_f4_facade_plan_durchreichung PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_F4_THESIS_MIN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/planner_thesis_min.profile.xml\"")
comdare_attach_generated_catalog(test_t2a_f4_facade_plan_durchreichung)
add_test(NAME test_t2a_f4_facade_plan_durchreichung COMMAND test_t2a_f4_facade_plan_durchreichung)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_t2a_f4_facade_plan_durchreichung)
set_tests_properties(test_t2a_f4_facade_plan_durchreichung PROPERTIES
    LABELS "bestandslog;bau_filter;facade;standalone")

# D3-7b-RIEGEL (2026-08-11) -- die GEGENSEITIGE AUSSCHLIESSUNG der zwei Lauf-Modi, als Code statt als
# Kommentar. Bis zur Heilung behaupteten DREI Kommentar-Stellen (RunProfileArgs, LazyRunConfig, die
# Bilanz-Zeile), provision_only und pruef_only koennten nicht zugleich gesetzt sein -- 0 Zeilen Code
# setzten das durch, und der erreichbare Doppel-Zustand fuhr still einen Provision-Lauf, der sich als
# Pruef-Lauf meldete. Der Test faehrt BEIDE Ebenen (Fassade run_profile / Iterator
# run_lazy_static_then_dynamic) und BEIDE Richtungen (verweigert im Konflikt / verweigert NICHT ohne).
# Include-/Link-/Define-Satz = test_t2a_f4_facade_plan_durchreichung direkt darueber (er zieht
# denselben Entry-Header samt Katalog und Ergebnis-Mappe); der einzige Unterschied ist gtest statt
# eigenem main, deshalb comdare_add_test.
comdare_add_test(test_d37b_modus_riegel
    SOURCES test_d37b_modus_riegel.cpp)
target_include_directories(test_d37b_modus_riegel PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_d37b_modus_riegel PRIVATE Boost::mp11
    comdare::anatomy_module_loader comdare::workload_driver comdare_builder_xml_config_parser
    comdare::lager_ablage)
target_compile_definitions(test_d37b_modus_riegel PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
comdare_attach_generated_catalog(test_d37b_modus_riegel)
set_tests_properties(test_d37b_modus_riegel PROPERTIES
    LABELS "facade;iterator;d3-7b;riegel")

# F3-Testschuld (Ledger D-04, :3720-3721) -- der ITERATOR-LAUF mit dem EINEN REALEN
# bestand_key_of-Provider (bestandslog::make_fingerprint_key_fn ueber das real geschriebene
# .fingerprint-Sidecar) und einem WERFENDEN CachePushFn (at::ArtefaktPushFehler, der Kontrakt-Typ
# seit TP1FK1-B2). Deckt die Kette Sidecar -> Provider -> observe -> Wurf -> AsyncPushPump::
# failed_dirs -> bestand_eintragspfad -> discard_fresh_with_pfad_prefix -> flush, die weder
# test_g3_artifact_cache_transport (Provider isoliert) noch test_tp1_planer_filter_iterator
# (Verwurf an der Funktions-Naht, synthetische Keys) im LAUF belegt hat. Gleicher Include-/Define-/
# Link-Satz wie test_tp1_planer_filter_iterator (derselbe Host-Treiber-Header, echter run_lazy-Aufruf).
add_executable(test_f3_lager_key_provider_iterator "test_f3_lager_key_provider_iterator.cpp")
target_compile_features(test_f3_lager_key_provider_iterator PRIVATE cxx_std_23)
target_include_directories(test_f3_lager_key_provider_iterator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_f3_lager_key_provider_iterator PRIVATE Boost::mp11
    comdare::anatomy_module_loader comdare::workload_driver)
comdare_set_platform_defines(test_f3_lager_key_provider_iterator)
target_compile_definitions(test_f3_lager_key_provider_iterator PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_f3_lager_key_provider_iterator COMMAND test_f3_lager_key_provider_iterator)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_f3_lager_key_provider_iterator)
set_tests_properties(test_f3_lager_key_provider_iterator PROPERTIES LABELS "bestandslog;bau_filter;standalone")

# LAG-P2 (2026-08-09) -- das MESSWERT-GENUS des Lagers. Schwester-Ziel zu test_f3_lager_key_provider_
# iterator: dort der reale bestand_key_of (Binary-Genus), hier der reale mess_bestand_key_of
# (measurement-Genus) samt Rueckschrieb und dem Owner-SKIP vom 08.08.2026. Braucht zusaetzlich
# profile_facade (RunProfileArgs) fuer die compile-harte Typ-Gleichheit der Drei-Schicht-Naht --
# deshalb der generated_source_catalog-Anschluss wie bei den anderen profile_run_entry-Konsumenten.
# KEIN contract-Label: der Test faehrt damit in test:unit (das seit R4 nur noch -LE pmc ausschliesst).
add_executable(test_lagp2_messwert_genus "test_lagp2_messwert_genus.cpp")
target_compile_features(test_lagp2_messwert_genus PRIVATE cxx_std_23)
target_include_directories(test_lagp2_messwert_genus PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_lagp2_messwert_genus PRIVATE Boost::mp11
    comdare::anatomy_module_loader comdare::workload_driver comdare_builder_xml_config_parser
    # A9-S5, Schwesterstelle zu test_t2a_f4_facade_plan_durchreichung: derselbe ODR-Use ueber
    # <profile_facade/profile_run_entry.hpp> (Zeile 51 dieser Testdatei).
    comdare::lager_ablage)
comdare_set_platform_defines(test_lagp2_messwert_genus)
target_compile_definitions(test_lagp2_messwert_genus PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
comdare_attach_generated_catalog(test_lagp2_messwert_genus)
add_test(NAME test_lagp2_messwert_genus COMMAND test_lagp2_messwert_genus)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_lagp2_messwert_genus)
set_tests_properties(test_lagp2_messwert_genus PROPERTIES LABELS "bestandslog;messwert_genus;standalone")

# B-3 + SCHEMA-FREEZE STUFE 1 (2026-08-09) -- die Schema-Hoheit der Mess-Ausgabe wird eingefroren.
#
# GEGENSTAND: die Spaltenliste. Sie ist der Vertrag, an dem die Auswerte-Stufen, die Resume-Wache, das
# Lager, die xlsx-Mappe und das PDF haengen. Bis heute schuetzte ihn nur gute Absicht.
#
# Der Test faehrt DREI Bissproben (Spalte zu / Spalte weg / zwei vertauscht) BEVOR er das echte Schema
# gruen meldet -- eine Wache, deren Biss nie vorgefuehrt wurde, ist von einer blinden nicht zu
# unterscheiden. Die Koeder sind je Lauf frisch gewuerfelt (Startwert steht in der Ausgabe).
#
# Include-/Link-Satz wie test_lazy_resume_binary: die TU zieht den ECHTEN Erzeuger
# cache_engine_builder_iterator.hpp (lazy_csv_header) statt eines Nachbaus; measurement_snapshot.hpp
# bringt die workload_driver-Kante mit (deshalb comdare::workload_driver, anders als bei
# test_lazy_resume_binary).
add_executable(test_b3_schema_freeze_stufe1 "test_b3_schema_freeze_stufe1.cpp")
target_compile_features(test_b3_schema_freeze_stufe1 PRIVATE cxx_std_23)
target_include_directories(test_b3_schema_freeze_stufe1 PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_b3_schema_freeze_stufe1 PRIVATE Boost::mp11
    comdare::workload_driver)
comdare_set_platform_defines(test_b3_schema_freeze_stufe1)
target_compile_definitions(test_b3_schema_freeze_stufe1 PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_b3_schema_freeze_stufe1 COMMAND test_b3_schema_freeze_stufe1)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_b3_schema_freeze_stufe1)
set_tests_properties(test_b3_schema_freeze_stufe1 PROPERTIES LABELS "contract;standalone;schema_freeze;b3")

# MT-L3 (2026-08-10) -- das UNABHAENGIGE Schema-Orakel der WIDE-Mess-CSV, als GOOGLE TEST.
#
# BEFUND, am Objekt mit zwei Werkzeugen gemessen: 33 Stellen in 19 TUs unter tests/ rufen
# lazy_csv_header(), und JEDE zieht ihr Soll aus dem Erzeuger selbst. Genau 1 Testdatei fuehrt den
# Voll-Header als Literal -- und das ist ein absichtlich VERALTETES Resume-Koeder-Schema, kein Orakel.
#
# ZWEI QUELLEN, ABSICHTLICH ZWEI UEBERSETZUNGSEINHEITEN:
#   mt_l3_wide_schema_gefroren.cpp  traegt das eingefrorene Byte-Literal und inkludiert <cstddef> und
#                                   <string_view> -- SONST NICHTS. Der Erzeuger ist dort weder direkt
#                                   noch mittelbar sichtbar. Genau das macht `soll = lazy_csv_header()`
#                                   nicht nur verboten, sondern unmoeglich: wer es doch wollte, muesste
#                                   erst die Include-Liste aendern, und das steht im Diff.
#   test_mt_l3_wide_schema_orakel.cpp  zieht den ECHTEN Erzeuger und haelt ihn dagegen.
# Waeren beide EINE TU, koennte der Erzeuger ueber den gemeinsamen Include-Baum in das Orakel
# zurueckwandern -- die Trennung ist die Zusicherung, nicht der Kommentar darueber.
#
# Include-/Link-Satz wie test_b3_schema_freeze_stufe1 (identische Erzeuger-Kette), zusaetzlich gtest
# ueber comdare_add_test -- Debug UND Release, kein Shell-Skript.
comdare_add_test(test_mt_l3_wide_schema_orakel
    SOURCES test_mt_l3_wide_schema_orakel.cpp mt_l3_wide_schema_gefroren.cpp)
target_include_directories(test_mt_l3_wide_schema_orakel PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_mt_l3_wide_schema_orakel PRIVATE Boost::mp11
    comdare::workload_driver)
target_compile_definitions(test_mt_l3_wide_schema_orakel PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_mt_l3_wide_schema_orakel PROPERTIES
    LABELS "contract;gtest;schema_freeze;mt_l3")

# VOR-PUSH-GATE (2026-08-10): die Testwerkbank des Werkzeugs tools/vor_push_gate.
# KEIN LIBRARIES-Satz und KEIN generated-Include -- der Kern vor_push_gate.hpp zieht nur die
# Standardbibliothek. Das ist die tragende Eigenschaft des Werkzeugs (es laeuft VOR einem Bau)
# und macht diesen Test zugleich zu einem der billigsten der Suite.
# Die Auswahl von test:unit ist '-LE pmc' -- dieser Test traegt kein pmc-Label und ist damit
# ohne Eintrag im Abdeckungs-Manifest (scripts/ci_test_coverage_manifest.sh) gedeckt.
comdare_add_test(test_vor_push_gate SOURCES test_vor_push_gate.cpp)
target_include_directories(test_vor_push_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/tools/vor_push_gate")
set_tests_properties(test_vor_push_gate PROPERTIES
    LABELS "gtest;ci_wache;vor_push_gate")

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

# W5-KERN (2026-08-05, Owner-R5) -- die Flaechen-TU des status-RUECK-LESERS
# (profile_facade/planner/planner_status_reader.hpp + progress_cursor_reader.hpp).
#
# Include-/Define-Satz wie test_lazy_resume_binary, WEIL die Fixtures aus den ECHTEN Schreib-Formen entstehen:
# der Resume-Stempel aus lazy_resume_stamp_prefix + kLazyResumeRowsKey, der CSV-Kopf aus lazy_csv_header --
# beides lebt im (schweren) Iterator-Header. Der Status-LESER selbst ist leicht; nur die Fixture-Seite zieht
# die Substanz herein, damit die TU keine Format-Kopie fuehrt.
# KEIN Netz: das Bestandslog kommt als lokaler emit_document-String (parse-Roundtrip).
add_executable(test_w5_status_reader "test_w5_status_reader.cpp")
target_compile_features(test_w5_status_reader PRIVATE cxx_std_23)
target_include_directories(test_w5_status_reader PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_w5_status_reader PRIVATE Boost::mp11)
comdare_set_platform_defines(test_w5_status_reader)
target_compile_definitions(test_w5_status_reader PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_w5_status_reader COMMAND test_w5_status_reader)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_w5_status_reader)
set_tests_properties(test_w5_status_reader PROPERTIES LABELS "w5;planner;status;standalone")

# Welle 5 (W5-C, §38-Dock-Protokoll, 2026-07-19) - zwei Gates nach der Layering-Verlagerung:
#   test_experiment_dock_payload -> TEIL 1 (planner): ExperimentSubtreePayload <-> R5-XML Byte-Roundtrip + Negativ.
#   test_progress_delta          -> TEIL 2 (builder-Leaf): der Fortschritts-Delta-Kanal gegen die REALE
#                                   StaticBinaryView-Folge (compute_progress_deltas freistehend, done 1x).
# Bewusst leicht: header-only, KEIN run_lazy_static_then_dynamic (AnatomyModuleLoader/dlopen) -- gleicher Include-/
# Define-Satz wie test_lazy_resume_binary. libs/common traegt die common-DOM (xml_reader) des R5-Parsers.
add_executable(test_experiment_dock_payload "test_experiment_dock_payload.cpp")
target_compile_features(test_experiment_dock_payload PRIVATE cxx_std_23)
target_include_directories(test_experiment_dock_payload PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_experiment_dock_payload PRIVATE Boost::mp11)
comdare_set_platform_defines(test_experiment_dock_payload)
target_compile_definitions(test_experiment_dock_payload PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_experiment_dock_payload COMMAND test_experiment_dock_payload)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_experiment_dock_payload)
set_tests_properties(test_experiment_dock_payload PROPERTIES LABELS "contract;standalone;harness;welle5;dock")

# Warnungs-Review Runde 2b (clang ueber den Test-Bau, 09.08.2026) -- Wire-Totalitaet des AxisKind-Kanals.
# Der Nachbar-Test darueber ist ein plain-main-Harnisch und wird per nacktem add_executable gebaut, traegt
# also KEINE Hauswarnstufe; genau deshalb blieb dort ein -Wswitch mit drei unbehandelten Meta-Meta-Realms
# unsichtbar (experiment_dock_payload.hpp:77). Dieses Gate ist ein GOOGLE TEST und laeuft ueber
# comdare_add_test -- damit traegt es COMDARE_set_default_warnings und meldet den Rueckfall selbst.
comdare_add_test(test_axis_kind_wire_totalitaet
    SOURCES test_axis_kind_wire_totalitaet.cpp
    LIBRARIES Boost::mp11)
target_include_directories(test_axis_kind_wire_totalitaet PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_compile_definitions(test_axis_kind_wire_totalitaet PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_axis_kind_wire_totalitaet PROPERTIES LABELS "contract;welle5;dock;warnungs-review")

# W5-C TEIL 2 (builder-Leaf progress_delta.hpp) - Delta-Rekonstruktion gegen die reale StaticBinaryView (done 1x).
add_executable(test_progress_delta "test_progress_delta.cpp")
target_compile_features(test_progress_delta PRIVATE cxx_std_23)
target_include_directories(test_progress_delta PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_progress_delta PRIVATE Boost::mp11)
comdare_set_platform_defines(test_progress_delta)
target_compile_definitions(test_progress_delta PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_progress_delta COMMAND test_progress_delta)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_progress_delta)
set_tests_properties(test_progress_delta PROPERTIES LABELS "contract;standalone;harness;welle5;dock")

# W5-C+ (§36.1 Zellen-Locking, 2026-07-19) - Zaehl-Gate fuer den GN-Zellen-Filter der opt×simd-Delegations-Naht
# (profile_facade/gn_cell_filter.hpp = Single-Source). Filter gesetzt -> genau 1 Perm; leer -> alle Perms; ISA-Gate.
# Bewusst umbrella-FREI (nur der Leaf-Header, reines stdlib) -> kein Boost, kein Katalog, KEIN DLL-Bau im Test.
add_executable(test_gn_cell_filter "test_gn_cell_filter.cpp")
target_compile_features(test_gn_cell_filter PRIVATE cxx_std_23)
target_include_directories(test_gn_cell_filter PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade")
comdare_set_platform_defines(test_gn_cell_filter)
add_test(NAME test_gn_cell_filter COMMAND test_gn_cell_filter)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_gn_cell_filter)
set_tests_properties(test_gn_cell_filter PROPERTIES LABELS "contract;standalone;welle5;gn-cell")
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

# STRUKT-R ORG-18 (2026-07-26): Gate der 18. Organ-Haupt-Achse persistence_target. Deckt den Owner-Pflicht-
# Beweis der XML-/option()-DEAKTIVIERBARKEIT (Q-1-Zusatz) end-to-end ab -- CMake-option -> flags-Header ->
# Baustein::enabled -> mp_filter -> EnabledTargets -> Katalog-Kardinalitaet 2^17 -> Registry-XML (genau 1
# Baustein, der deaktivierte Name fehlt) -- plus die belegte mp_take_c-Falle (K17==1), die Lese-seitige
# Alt-id-Normalisierung inkl. Byte-Pin auf die unveraenderte Alt-Mess-CSV, und die Concept-Erfuellung BEIDER
# Observable-Huellen (persistence_target + der nachgezogene io_dispatch-Alt-Defekt).
comdare_add_test(test_org18_persistence_target
    SOURCES test_org18_persistence_target.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_org18_persistence_target)
target_include_directories(test_org18_persistence_target PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_org18_persistence_target PRIVATE Boost::mp11)
target_compile_definitions(test_org18_persistence_target PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1
    "COMDARE_ORG18_AXIS_REGISTRY_XML=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml\""
    "COMDARE_ORG18_TIER150_CSV=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/tier150_measurements.csv\"")
set_tests_properties(test_org18_persistence_target PROPERTIES LABELS "contract;registry;org18")

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
    "COMDARE_GOLDEN_320_BYTE_GUARD_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\""
    "COMDARE_GOLDEN_320_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\"") # GN-8 (§33): _IDS = DEPRECATED-Alias -> _BYTE_GUARD_IDS; 320 = Byte-Wache (byte-identischer Gate-Waechter), NICHT der Systembeweis (= new-golden-Voll-Konfig)
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
    "COMDARE_GOLDEN_320_BYTE_GUARD_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\""
    "COMDARE_GOLDEN_320_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\"") # GN-8 (§33): _IDS = DEPRECATED-Alias -> _BYTE_GUARD_IDS; 320 = Byte-Wache (byte-identischer Gate-Waechter), NICHT der Systembeweis (= new-golden-Voll-Konfig)
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
    "COMDARE_GOLDEN_320_BYTE_GUARD_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\""
    "COMDARE_GOLDEN_320_IDS=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt\"") # GN-8 (§33): _IDS = DEPRECATED-Alias -> _BYTE_GUARD_IDS; 320 = Byte-Wache (byte-identischer Gate-Waechter), NICHT der Systembeweis (= new-golden-Voll-Konfig)
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

# KERN #48-S4 (2026-07-22) -- ctest-Gate der Minimal-Parser-/Validator-Naht der vier neuen comdare_experiment-v2-
# Elemente (machines / axis@pruefling / merge=fulljoin / storage). ADDITIV: eigene Fixture experiment_golden_kern.xml
# + eigenes Source-File; beruehrt KEINEN bestehenden Parser-/Projection-/Director-Pin. Fixture wird NUR GELESEN.
comdare_add_test(test_experiment_kern_seam
    SOURCES thesis_tiere/test_experiment_kern_seam.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_experiment_kern_seam)
target_include_directories(test_experiment_kern_seam PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_experiment_kern_seam PRIVATE Boost::mp11)
target_compile_definitions(test_experiment_kern_seam PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_EXPERIMENT_GOLDEN_KERN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/experiment_golden_kern.xml\"")
set_tests_properties(test_experiment_kern_seam PROPERTIES LABELS "contract;profiles;e4;incd")

# BRÜCKE I3 (2026-07-16) — ctest-Gate der Projektion ExperimentProfile → (merge×lebewesen)-Pässe/Quellen-Map
# (sota_catalog.hpp::project_experiment_to_sota_passes, E5/Fork-5 (b) schmale Overloads). REINER Enumerations-/
# Render-Schritt der Fork-A-Brücke — KEIN DLL-Bau, KEINE Messung. Beweist (a) je Phase die WÖRTLICH erwarteten
# view-binary_id-Keys (Golden: Stufe1→Reihe A/7, Stufe2→Reihe A/6, Stufe3→Reihe B/6 = 19 Pässe), (b) nicht-leere
# Quelltexte je Pass (COMDARE_DEFINE_ANATOMY_MODULE-Marker) 1:1 über view_binary_id, (c) nullopt-Paare ehrlich
# ausgelassen (prt_art degeneriert unter Stufe2/Stufe3 ⇒ kein Phantom-Key/-Pass). experiment_golden.xml wird NUR
# GELESEN. sota_catalog.hpp zieht die Compositions ⇒ Boost::mp11 + Achsen-Generated-Dirs wie test_sota_st2_dedup.
comdare_add_test(test_experiment_projection
    SOURCES thesis_tiere/test_experiment_projection.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_experiment_projection)
target_include_directories(test_experiment_projection PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_experiment_projection PRIVATE Boost::mp11)
target_compile_definitions(test_experiment_projection PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_EXPERIMENT_GOLDEN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/experiment_golden.xml\"")
set_tests_properties(test_experiment_projection PROPERTIES LABELS "contract;profiles;e4;incd")

# KERN-B K5 (Section 59, 2026-07-20) -- Gate der deklarativen Merge-Naht (merge_plan_from_profile) + des
# direktiven-getriebenen Emitters (render_directive_merge_module_source). Reine Daten-/Render-Schritte (kein
# DLL-Bau/keine Messung); sota_catalog.hpp zieht die Compositions => Boost::mp11 + Achsen-Generated-Dirs wie
# test_experiment_projection.
comdare_add_test(test_merge_plan_directive
    SOURCES thesis_tiere/test_merge_plan_directive.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_merge_plan_directive)
target_include_directories(test_merge_plan_directive PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_merge_plan_directive PRIVATE Boost::mp11)
target_compile_definitions(test_merge_plan_directive PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_merge_plan_directive PROPERTIES LABELS "contract;profiles;kernb;merge")

# PAKET W3-B / Phase-1-I1 (2026-07-19) — Contract-Test des ExperimentPlanDirector (GoF Director + Builder,
# profile_facade/planner/experiment_plan_director.hpp) + des Registry-Trio-Lesers (validate_profile.hpp
# read_axis_registry_trio). Beweist LITERAL: (T) die 3 Art-Registries (Organ/System/Mess) laden ueber DIESELBE
# read_axis_registry-Naht -> 17/5/16-Zaehlung; (A) Thesis-min (keine <system_axes>/<axis_sweeps>) = 1 Identitaets-
# Perm x 1 Basis-Pass; (B) all_axes_golden = 2x2 Perms x 18 Sweep-Passes = 72 Schritte; (C/C') der --dump-plan-
# Text ist byte-deterministisch (2 Laeufe gleich, Thesis + Experiment); (D) der Experiment-Kanal enumeriert je
# Perm die Bruecke-I3-Phasen (3 Phasen -> 19 Passes); (E) der Plan-Kopf traegt die 3 Angebots-Quellen (Resolver-
# Vorstufe). REINER Enumerations-/Render-Schritt: KEIN Bau, KEINE Messung, KEIN run_lazy_static_then_dynamic,
# KEINE CSV (Anti-Phantom, golden-neutral). Alle Fixtures + die 3 Art-Registries werden NUR GELESEN (Single-
# Source per Define auf die realen/committeten Dateien; kein Kopie-Drift). sota_catalog.hpp zieht die
# Compositions => Boost::mp11 + Achsen-Generated-Dirs wie test_experiment_projection.
comdare_add_test(test_experiment_plan_director
    SOURCES thesis_tiere/test_experiment_plan_director.cpp
    LIBRARIES comdare_builder_xml_config_parser)
comdare_prepare_thesis_object_dir(test_experiment_plan_director)
target_include_directories(test_experiment_plan_director PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${CMAKE_CURRENT_SOURCE_DIR}/thesis_tiere"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_experiment_plan_director PRIVATE Boost::mp11)
target_compile_definitions(test_experiment_plan_director PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_PLANNER_THESIS_MIN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/planner_thesis_min.profile.xml\""
    "COMDARE_PLANNER_THESIS_ALL_AXES=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/all_axes_golden.profile.xml\""
    "COMDARE_EXPERIMENT_GOLDEN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/experiment_golden.xml\""
    "COMDARE_CE_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml\""
    "COMDARE_SYSTEM_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement/system_axis_registry.xml\""
    "COMDARE_MEASUREMENT_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement/measurement_axis_registry.xml\"")
# Fehlende Bau-Graph-Kante (gefunden 2026-08-06 bei A1-NB2, reproduzierbar unter -j32):
# thesis_tiere/test_experiment_plan_director.cpp zieht ueber profile_run_entry.hpp
# generated_source_catalog.hpp (wie test_lazy_adhoc_source_gen/test_limits_entkopplung_vorstufe/
# test_t2a_f4_facade_plan_durchreichung), hatte aber als einziges der 5 Ziele die Kante NICHT.
# Ohne sie kann Ninja die P1689-Scan-Stufe dieses Targets parallel zum Generator-Custom-Command
# einplanen -> "generated_source_catalog.hpp: No such file or directory" bei ungluecklicher
# Bau-Reihenfolge. comdare_attach_generated_catalog(...) macht die Kante explizit.
comdare_attach_generated_catalog(test_experiment_plan_director)
set_tests_properties(test_experiment_plan_director PROPERTIES LABELS "contract;profiles;e4;planner")

# M-CE-10 (Voll-Review 2026-07-13; per-Host-Auffaecherung 2026-07-14) — ctest-Gate der
# Stufe2_PrueflingReplace-Materialisierung im sota_catalog. KORREKTUR F23 (2026-07-16): die fruehere
# Beschreibung ("St2 = EINE Binary/HOT-Pilot, Dedup zu 1 Pass, h2_lebewesen == 'hot', 14 gezaehlt") war die
# VOR-M-CE-10-Semantik — der Test asserted seit der per-Host-Auffaecherung das Gegenteil. Beweist (Wortlaut
# test_sota_st2_dedup.cpp-Kopf): (1) PER-HOST-DISTINKT — mehrere St2-Deklarationen mit verschiedenen
# lebewesen ⇒ je Host EIN distinkter Pass, NICHT mehr zu 1 kollabiert; (2) H2-Attribution host-dominant mit
# host == angefragtem lebewesen (EXPECT_EQ(p.h2_lebewesen, p.lebewesen)); (3) das gepinnte m3v2_study
# (7 St1 + 7 St2 + 7 St3) liefert 19 Paesse mit 19 DISTINKTEN view_binary_ids (7 St1 + 6 St2 + 6 St3,
# prt_art degeneriert); (4) fairness-Varianten bleiben getrennte Paesse; (5) echte Dedup bleibt (gleicher
# Host ⇒ 1 Pass). m3v2_study.profile.xml + sota_h2_scores.xml werden NUR GELESEN (TABU-Wache).
# comdare_add_test registriert auto in COMDARE_TEST_TARGETS.
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
    "COMDARE_THESIS_PROFILES_DIR=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles\""
    # S3 P-RESOLVER (2026-07-20): die 3 Art-Registries fuer resolve_axis_refs_against_trio (REALES Angebot lesen
    # statt eine Achsenliste hartkodieren -- Single-Source, kein Drift). Deckungsgleich zu test_experiment_plan_director.
    "COMDARE_CE_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml\""
    "COMDARE_SYSTEM_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement/system_axis_registry.xml\""
    "COMDARE_MEASUREMENT_AXIS_REGISTRY=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement/measurement_axis_registry.xml\"")
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
# E-24 C10 (a) -- die beiden NEUEN Modul-Quellen des Roundtrip-Beweises. Sie haengen sich BEWUSST an
# dieselbe boost-freie Schleife wie perm_set_d9/d10/d11: eine Sonder-Registrierung waere ein zweiter
# Bau-Weg fuer dieselbe Sache.
#   perm_adapter_d12       das bis hier FEHLENDE vierte Container-Genus als DLL.
#   perm_set_defekt_d13    das Negativ-Beweismittel des Konformitaets-Gates (dock_status 4).
set(_src_perm_adapter_d12          genus_module_adapter.cpp)
set(_src_perm_set_defekt_d13       genus_module_set_defekt.cpp)
# E-24 C10 (b) / G5 -- die beiden ALT-MODULE der Lade-Wache. Der Loader traegt ZWEI Schloesser (Magic,
# dann Major) und prueft sie in fester Reihenfolge; ein echtes Alt-Modul bewegt beide und erreicht das
# zweite nie. Deshalb zwei Quellen, damit jedes Schloss EINZELN beobachtbar ist.
#   perm_alt_major7        Magic ".A7." + Version 7.0 -- herkunfts-treue Quelle der gesicherten
#                          Fixture major7-referenz-44bcda99.so (Schloss 1).
#   perm_major7_neue_magic HEUTIGE Magic + Version (Host-Major - 1) -- Isolierungs-Probe (Schloss 2).
set(_src_perm_alt_major7           genus_module_alt_major7.cpp)
set(_src_perm_major7_neue_magic    genus_module_major7_neue_magic.cpp)
foreach(_dll IN ITEMS perm_set_d9 perm_sequence_d10 perm_view_d11 perm_buildvariant_avx512 perm_buildvariant_avx2
                      perm_adapter_d12 perm_set_defekt_d13 perm_alt_major7 perm_major7_neue_magic)
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

# -----------------------------------------------------------------------------
# E-24 C10 (Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.2-C10) --
# die Roundtrip-Beweise. DLL-getriebene TUs; sie brauchen ueber test_dgenus_dll hinaus die
# Achsen-Registries (Docks/Engines -> Boost::mp11 + generated-Flags), deshalb eine eigene Registrierung
# statt der schlanken Loader-Zeilen oben.
#   (a) test_e24_c10_genus_dll_roundtrip -- V5-Vertrag ueber vier ECHT geladene Container-DLLs + die
#       beiden Negativ-Proben (dock_status 2/4) + measure_genus_sequential-Mischlauf.
#   (b) test_e24_c10_g5_lade_wache -- der Major-Wechsel als Lade-Wache (beide Schloesser einzeln,
#       Major-8-Roundtrip gruen) + der ceb-contract-Beleg des --version-Blocks.
# -----------------------------------------------------------------------------
foreach(_c10 IN ITEMS test_e24_c10_genus_dll_roundtrip test_e24_c10_g5_lade_wache)
    add_executable(${_c10} "${_c10}.cpp")
    target_compile_features(${_c10} PRIVATE cxx_std_23)
    target_include_directories(${_c10} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_c10} PRIVATE Boost::mp11 comdare::anatomy_module_loader)
    comdare_set_platform_defines(${_c10})
    add_dependencies(${_c10} perm_set_d9 perm_sequence_d10 perm_view_d11 perm_adapter_d12 perm_set_defekt_d13
                             perm_alt_major7 perm_major7_neue_magic)
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_c10})
endforeach()

add_test(NAME test_e24_c10_genus_dll_roundtrip COMMAND test_e24_c10_genus_dll_roundtrip
    "--gut=$<TARGET_FILE:perm_set_d9>" "--gut=$<TARGET_FILE:perm_sequence_d10>"
    "--gut=$<TARGET_FILE:perm_adapter_d12>" "--gut=$<TARGET_FILE:perm_view_d11>"
    "--defekt-set=$<TARGET_FILE:perm_set_defekt_d13>")
set_tests_properties(test_e24_c10_genus_dll_roundtrip PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip;e24")

# G5: die gesicherte Original-Fixture (/home/comdare/e24-g5-fixture/major7-referenz-44bcda99.so) wird
# BEWUSST NICHT als ctest-Argument verdrahtet -- sie liegt ausserhalb des Repos und waere eine
# Host-Abhaengigkeit im CI. ctest faehrt die committete Nachbildung perm_alt_major7; der Lauf gegen das
# Original steht als literaler Beleg im Fenster-Report (optionales --gesicherte-fixture=<pfad>).
add_test(NAME test_e24_c10_g5_lade_wache COMMAND test_e24_c10_g5_lade_wache
    "--alt=$<TARGET_FILE:perm_alt_major7>" "--alt-neue-magic=$<TARGET_FILE:perm_major7_neue_magic>"
    "--neu=$<TARGET_FILE:perm_set_d9>")
set_tests_properties(test_e24_c10_g5_lade_wache PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip;e24")

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

# MT-L4 (09.08.2026): D4b.2 nachregistriert -- der ECHTE Container-DLL-Round-Trip. Er lag seit
# 02.06.2026 unregistriert im Baum, 68 Tage, laenger als jede andere unregistrierte Testdatei.
# DASS ER FEHLTE, STAND SEIT DEM ERSTEN TAG IM REPO: der Kopf seines bereits registrierten
# Zwillings test_d4b_container_adapter.cpp (D4b.1, in-process, tests/unit/CMakeLists.txt:2351)
# sagt woertlich "der echte .dll-Round-Trip folgt in D4b.2 (test_d4b_container_dll via
# AnatomyModuleLoader)". D4b.1 wurde verdrahtet, D4b.2 nicht -- die Schwesterstelle blieb liegen.
# Der Unterschied ist nicht kosmetisch: D4b.1 prueft den ABI-Adapter IN-PROCESS, D4b.2 prueft
# dieselbe Gattungs-API ueber die REALE .so-Grenze. Nur der zweite faengt einen Bruch, der erst
# beim Laden auftritt.
# DAS PILOT-MODUL EXISTIERT BEREITS und musste nicht gebaut werden: perm_adapter_d12 (:3972/:3983)
# ist die Adapter-Genus-Permutations-DLL, die test_e24_c10_genus_dll_roundtrip bei :4048 schon als
# "--gut="-Argument faehrt. Wiring-Muster deshalb 1:1 von dort: Pflicht-Argument per
# $<TARGET_FILE:...>, add_dependencies auf das DLL-Ziel. Der Test hat ein Pflicht-CLI-Argument
# (ohne Pfad Exit 2) -- ein add_test ohne Argument waere ein Daueralarm.
add_executable(test_d4b_container_dll "test_d4b_container_dll.cpp")
target_compile_features(test_d4b_container_dll PRIVATE cxx_std_23)
target_include_directories(test_d4b_container_dll PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_d4b_container_dll PRIVATE Boost::mp11 comdare::anatomy_module_loader)
comdare_set_platform_defines(test_d4b_container_dll)
add_dependencies(test_d4b_container_dll perm_adapter_d12)
add_test(NAME test_d4b_container_dll COMMAND test_d4b_container_dll
    "$<TARGET_FILE:perm_adapter_d12>")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_d4b_container_dll)
set_tests_properties(test_d4b_container_dll PROPERTIES LABELS "goal_v6;phase_d;dll_roundtrip;d4b" TIMEOUT 60)

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

# -----------------------------------------------------------------------------
# W3-KERN (2026-08-05, Owner-GO R3) -- POST-COMPILE-TESTATE der MESS-Interfaces am Pruef-Dock.
#
# (a) test_w3_mess_interface_testate_dock -- faehrt import -> GATE -> Testate gegen ZWEI ECHTE, per dlopen
#     geladene Module. Traeger sind BESTEHENDE SHARED-Targets (KEIN neuer DLL-Bau):
#     anatomy_codegen_pilot_wormhole_shared (die reale codegen-Komposition) + perm_adhoc_buildvariant
#     (zweiter, andersartiger Vertreter). Beide Pfade reisen als ARGUMENTE ($<TARGET_FILE:...>, Muster
#     test_d13_dll_runtime_measure) -- bewusst NICHT als -D-Pfad-Makros: die frisch belegte
#     cppcheck-unknownMacro-Falle (Makro in String-Literal-Adjazenz; lint:static laeuft OHNE CMake-Defines)
#     wird so strukturell umgangen statt nachtraeglich geheilt.
#     Include-/Link-Rezeptur = test_v41_anatomy_f15_measurement (die TU pinnt via static_assert den REALEN
#     SearchAlgorithmAbiAdapter<SearchAlgorithmAnatomy<WormholeComposition>> und braucht darum den
#     Anatomie-Umbrella + Boost::mp11 + die generierten Achsen-Dirs).
#
# (b) test_w3_mess_interface_testate_biss -- der beidseitige Biss: jede Testat-Funktion einmal gegen eine
#     korrekte und einmal gegen eine gezielt defekte Huelle. Standalone/leicht (nur ABI-Header + std),
#     KEIN DLL-Bau noetig, weil die Testat-Funktionen Interface-Referenzen nehmen.
#
# MESSUNGS-GEGATET: beide TUs pruefen Sub-Interfaces, die der ABI-Adapter NUR unter COMDARE_MEASUREMENT_ON
# erbt. In einem Release-/funktional-only-Baum waeren die dynamic_casts zu Recht nullptr -- dort duerfen die
# TUs gar nicht erst registriert werden, sonst entstuende ein Fehlalarm statt einer Aussage.
if(COMDARE_MEASUREMENT_MODE)
    add_executable(test_w3_mess_interface_testate_dock test_w3_mess_interface_testate_dock.cpp)
    target_include_directories(test_w3_mess_interface_testate_dock PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_compile_features(test_w3_mess_interface_testate_dock PRIVATE cxx_std_23)
    target_link_libraries(test_w3_mess_interface_testate_dock PRIVATE Boost::mp11 comdare::anatomy_module_loader)
    add_dependencies(test_w3_mess_interface_testate_dock
        anatomy_codegen_pilot_wormhole_shared perm_adhoc_buildvariant)
    add_test(NAME test_w3_mess_interface_testate_dock COMMAND test_w3_mess_interface_testate_dock
        "$<TARGET_FILE:anatomy_codegen_pilot_wormhole_shared>" "$<TARGET_FILE:perm_adhoc_buildvariant>")
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_w3_mess_interface_testate_dock)
    set_tests_properties(test_w3_mess_interface_testate_dock
        PROPERTIES LABELS "w3;pruef_dock;measured;dll_roundtrip" TIMEOUT 300)

    add_executable(test_w3_mess_interface_testate_biss test_w3_mess_interface_testate_biss.cpp)
    target_include_directories(test_w3_mess_interface_testate_biss PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
    target_compile_features(test_w3_mess_interface_testate_biss PRIVATE cxx_std_23)
    add_test(NAME test_w3_mess_interface_testate_biss COMMAND test_w3_mess_interface_testate_biss)
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_w3_mess_interface_testate_biss)
    set_tests_properties(test_w3_mess_interface_testate_biss
        PROPERTIES LABELS "w3;pruef_dock;measured;biss" TIMEOUT 120)

    message(STATUS "W3-KERN: Mess-Interface-Testate REGISTRIERT (dock + biss)")
else()
    message(STATUS "W3-KERN: Mess-Interface-Testate NICHT registriert (COMDARE_MEASUREMENT_MODE=OFF -- die "
        "messungs-gegateten Sub-Interfaces existieren in diesem Baum nicht)")
endif()

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
# (REV-DATA-07, WP-5 2026-07-16): Fixture 3 = Zell-Raster (unvollstaendiger Kandidat wird disqualifiziert).
# (WELLE C T-8, 2026-08-07): Fixture 4 = Pareto-Front-Bissbeweis (Einzelsieger-Pfad kuert eine DOMINIERTE
# Binary; zweite Probe: eine als MAX gefuehrte Zielgroesse kehrt die Front um).
add_test(NAME test_best_binary_selector_parse_rank COMMAND test_best_binary_selector_parse_rank
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/best_binary_wide.csv"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/best_binary_strict_values.csv"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/best_binary_cells.csv"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/best_binary_pareto.csv")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_best_binary_selector_parse_rank)
set_tests_properties(test_best_binary_selector_parse_rank PROPERTIES LABELS "tools;best_binary" TIMEOUT 60)

# 2026-07-20 (S1/A5, Section 32-F8 / Section 49): SYNTHETISCHE Verifikation des Hybrid-Break-Even-Vorbaus —
# curve_fit.hpp AxisSpline (Spline je Achse) + best_binary_selector.hpp Break-Even/Rueckwaerts-Wahl (self-
# contained, std-only) + decision_lambda_trees.hpp (Spline-Schnittpunkt-Entscheidungsbaum). GOLDEN-NEUTRAL
# (rein synthetische Kurven, keine #156-Messdaten). Linkt comdare_decision_lambda_trees INTERFACE (belegt die
# CMake-Anbindung) + kompiliert best_binary_selector.cpp direkt (engine-frei). Traegt zusaetzlich das K-5-
# Paritaets-Gate am neuen Selektor-Rand (static_assert gegen anatomy_module_abi_v1_decl.hpp).
add_executable(test_hybrid_spline_selector_scaffold
    "test_hybrid_spline_selector_scaffold.cpp"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector/best_binary_selector.cpp")
target_compile_features(test_hybrid_spline_selector_scaffold PRIVATE cxx_std_23)
target_include_directories(test_hybrid_spline_selector_scaffold PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector")
target_link_libraries(test_hybrid_spline_selector_scaffold PRIVATE comdare_decision_lambda_trees)
comdare_set_platform_defines(test_hybrid_spline_selector_scaffold)
target_compile_definitions(test_hybrid_spline_selector_scaffold PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_hybrid_spline_selector_scaffold COMMAND test_hybrid_spline_selector_scaffold)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_hybrid_spline_selector_scaffold)
set_tests_properties(test_hybrid_spline_selector_scaffold PROPERTIES LABELS "tools;best_binary;hybrid;curve_fit" TIMEOUT 60)

# Schicht E4 Paket B - XML-Eingang der Resource-Control-Felder bis zur E3-AxisLevel-Struktur.
add_executable(test_e4_contract_xml_to_axislevels "test_e4_contract_xml_to_axislevels.cpp")
target_compile_features(test_e4_contract_xml_to_axislevels PRIVATE cxx_std_23)
target_include_directories(test_e4_contract_xml_to_axislevels PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization")
target_link_libraries(test_e4_contract_xml_to_axislevels PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_e4_contract_xml_to_axislevels)
add_test(NAME test_e4_contract_xml_to_axislevels COMMAND test_e4_contract_xml_to_axislevels
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/e4_contract_profile.xml")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_e4_contract_xml_to_axislevels)
# Label BEWUSST ohne 'contract': ce test:unit laeuft mit -LE 'contract|pmc', und die dedizierten
# contract-Gates matchen nur explizite -R-Listen -- mit 'contract'-Label waere der Test CI-tot.
set_tests_properties(test_e4_contract_xml_to_axislevels PROPERTIES LABELS "profiles;e4")

# MT-L4 (09.08.2026): A9b-Guard nachregistriert. Er lag seit 26.07. unregistriert im Baum -- getrackt,
# uebersetzbar, in KEINER CMakeLists.txt genannt (gemessen: 0 Treffer). Die im Dateikopf genannte
# Begruendung ("Sammel-Registrierung am Join, laufende Welle ist Konflikt-Zone") ist mit dem Join
# erledigt; sein Nachbar RF-3 wurde aus demselben Grund am 02.08. nachgezogen (Owner-Entscheid E6).
# HIER, weil sein Kopf die Include-Wurzeln ausdruecklich als "die des Muster-Blocks von
# test_e4_contract_xml_to_axislevels" bezeichnet -- der steht direkt darueber. Zusaetzlich zu jenem
# Block braucht er cache_engine/include (validate_profile.hpp zieht cache_engine/measurement/*), die
# GENERIERTE Flags-Wurzel fuer Teil 4 und Boost::mp11. Der xml_reader ist header-only; die einzige
# fremde Uebersetzungseinheit ist der Parser, hier ueber sein Bibliotheks-Ziel statt als roher .cpp-Pfad.
# Kein gtest (eigenes main, Exit 0/1).
add_executable(test_a9b_active_deklaration_inert "test_a9b_active_deklaration_inert.cpp")
target_compile_features(test_a9b_active_deklaration_inert PRIVATE cxx_std_23)
target_include_directories(test_a9b_active_deklaration_inert PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(test_a9b_active_deklaration_inert PRIVATE
    Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_a9b_active_deklaration_inert)
add_test(NAME test_a9b_active_deklaration_inert COMMAND test_a9b_active_deklaration_inert)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_a9b_active_deklaration_inert)
set_tests_properties(test_a9b_active_deklaration_inert PROPERTIES LABELS "profiles;a9b;registry" TIMEOUT 60)

# T-1 (2026-08-09): DIE ZWEI PARSES DESSELBEN PROFILS, DIE DASSELBE SAGEN MUESSEN.
# profile_run_facade.cpp entscheidet aus SEINEM Parse ueber compile_for_perm, profile_run_entry.hpp aus
# SEINEM ueber die opt x simd-Perm-Schleife. Ueber EINEN Parse sind die beiden Ausdruecke De-Morgan-Duale
# (der Test misst das: 5 von 5 Profil-Formen gleich); getrennt hat sie die STRUKTUR -- zwei unabhaengige
# load_thesis_profile()-Aufrufe auf denselben Pfad zu zwei Zeitpunkten. Der Test faehrt genau das nach.
# Include-/Link-Satz wie test_e4_contract_xml_to_axislevels: er braucht nur den Parser-Datentyp und den
# neuen header-only Entscheidungs-Header (libs/cache_engine/profile_facade), sonst nichts.
# DREI ctest-EINTRAEGE: die Wache, der Mutant-Selbstbiss (faehrt den ROT-Beleg bei JEDEM Lauf mit, statt
# ihn als Prosa verjaehren zu lassen) -- T-7: ein Test, der in keinem Job faehrt, ist nicht gebaut.
add_executable(test_t1_system_achsen_eine_entscheidung "test_t1_system_achsen_eine_entscheidung.cpp")
target_compile_features(test_t1_system_achsen_eine_entscheidung PRIVATE cxx_std_23)
target_include_directories(test_t1_system_achsen_eine_entscheidung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization")
target_link_libraries(test_t1_system_achsen_eine_entscheidung PRIVATE
    Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_t1_system_achsen_eine_entscheidung)
add_test(NAME test_t1_system_achsen_eine_entscheidung
    COMMAND test_t1_system_achsen_eine_entscheidung)
add_test(NAME test_t1_system_achsen_eine_entscheidung_selbstbiss
    COMMAND test_t1_system_achsen_eine_entscheidung --selbstbiss)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_t1_system_achsen_eine_entscheidung)
set_tests_properties(test_t1_system_achsen_eine_entscheidung
    test_t1_system_achsen_eine_entscheidung_selbstbiss
    PROPERTIES LABELS "profiles;system_axes;standalone")

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
# Hermetik (CI-3): test_m_contract_system_axis_wurzel zieht transitiv
# topics/axis_command_base.hpp -> axis_06_allocator_* -> legacy_original_code_strategy_concept.hpp
# (boost::mp11). Wie alle boost-berührten Tests explizit an Boost::mp11 linken (vendored
# Include-Pfad; System-Boost maskiert die Lücke lokal, hermetische CI deckt sie auf).
target_link_libraries(test_m_contract_system_axis_wurzel PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_m_contract_system_axis_wurzel)
target_compile_definitions(test_m_contract_system_axis_wurzel PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_m_contract_system_axis_wurzel COMMAND test_m_contract_system_axis_wurzel)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m_contract_system_axis_wurzel)
set_tests_properties(test_m_contract_system_axis_wurzel PROPERTIES LABELS "m;blut")

# Querschnitt M - Section 40.a-E1: feingranularer SIMD-Flag-Katalog + Maschinen-Signaturen (header-only).
add_executable(test_m_simd_feature_flag_catalog "test_m_simd_feature_flag_catalog.cpp")
target_compile_features(test_m_simd_feature_flag_catalog PRIVATE cxx_std_23)
target_include_directories(test_m_simd_feature_flag_catalog PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
# Hermetik (CI-3): zieht transitiv topics/axis.hpp (boost::mp11) -- explizit an Boost::mp11 linken.
target_link_libraries(test_m_simd_feature_flag_catalog PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_m_simd_feature_flag_catalog)
target_compile_definitions(test_m_simd_feature_flag_catalog PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_m_simd_feature_flag_catalog COMMAND test_m_simd_feature_flag_catalog)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m_simd_feature_flag_catalog)
set_tests_properties(test_m_simd_feature_flag_catalog PROPERTIES LABELS "m;blut")

# Task #7 P1: DDR5-SPD-Byte-Parser (header-only, reine span-Funktionen, KEIN IO im Parser).
# Bewusst OHNE Boost::mp11: der Test zieht nur measurement/spd_ddr5_parser.hpp + axis_error.hpp +
# ram_frequency_reading.hpp, also keine topics/axis.hpp-Kette (verifiziert per -fsyntax-only).
add_executable(test_p1_spd_ddr5_parser "test_p1_spd_ddr5_parser.cpp")
target_compile_features(test_p1_spd_ddr5_parser PRIVATE cxx_std_23)
target_include_directories(test_p1_spd_ddr5_parser PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(test_p1_spd_ddr5_parser PRIVATE gtest gtest_main)
comdare_set_platform_defines(test_p1_spd_ddr5_parser)
target_compile_definitions(test_p1_spd_ddr5_parser PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_p1_spd_ddr5_parser COMMAND test_p1_spd_ddr5_parser)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_p1_spd_ddr5_parser)
set_tests_properties(test_p1_spd_ddr5_parser PROPERTIES LABELS "hwprobe;spd;measurement")

# Task #7 P2: die RT-Erhebungs-Kette + CT-Factory (Zelle ISA-Komplex x OS) und die RAM-Drift-
# Gegenprobe. Dieselbe leichte Include-Menge wie test_p1 -- die neuen Header ziehen nur die
# measurement-Kette (axis_error/ram_frequency_reading/spd_ddr5_parser/machine_identity + die beiden
# Achsen-Header), also KEIN Boost::mp11 und keine topics/axis.hpp-Kette (verifiziert per
# -fsyntax-only ohne mp11-Include-Pfad).
# Der Ketten-Test legt Wegwerf-Baeume unter temp_directory_path an (Muster wie
# test_best_binary_selector_parse_rank/test_a9b) und raeumt sie im Destruktor wieder ab; er liest
# KEINE Live-Hardware-Werte, sondern eingespielte Puffer (Auflage A9).
foreach(_p2_test test_p2_ram_probe_chain test_p2_ram_identity_verdict)
    add_executable(${_p2_test} "${_p2_test}.cpp")
    target_compile_features(${_p2_test} PRIVATE cxx_std_23)
    target_include_directories(${_p2_test} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated")
    target_link_libraries(${_p2_test} PRIVATE gtest gtest_main)
    comdare_set_platform_defines(${_p2_test})
    target_compile_definitions(${_p2_test} PRIVATE
        COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
    add_test(NAME ${_p2_test} COMMAND ${_p2_test})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_p2_test})
    set_tests_properties(${_p2_test} PROPERTIES LABELS "hwprobe;measurement")
endforeach()

# Paket OS-U3: prozessfreie RT-Erhebung von os_version/kernel/build per CT-Familien-Spezialisierung.
# Der Test fuehrt die K4-Fehlerproduzenten, unabhaengige Linux-Live-Gegenproben und die A-15-Stempel-
# Neutralitaet aus. Boost::mp11 und der builder-Include-Pfad folgen dem A14-Stempel-Guard, weil auch
# diese TU abi/anatomy_version_stamp.hpp direkt einbindet.
add_executable(test_os_u3_probe "test_os_u3_probe.cpp")
target_compile_features(test_os_u3_probe PRIVATE cxx_std_23)
target_include_directories(test_os_u3_probe PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(test_os_u3_probe PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_os_u3_probe)
target_compile_definitions(test_os_u3_probe PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_os_u3_probe COMMAND test_os_u3_probe)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_os_u3_probe)
set_tests_properties(test_os_u3_probe PROPERTIES LABELS "osprobe;measurement")

# Paket OD-10-RT: prozessfreie RT-Erhebung der target_isa-Unter-Achsen numa_node/page per
# CT-Familien-Spezialisierung. Der Test faehrt die tragende Parser-/Topologie-Logik HARDWARE-FREI gegen
# einen Fixture-sysfs-Baum im Test-Tempdir, belegt jede K4-Fehlerklasse einzeln und prueft die
# A-15-Stempel-Neutralitaet. Include-/Link-Menge identisch zu test_os_u3_probe: auch diese TU bindet
# abi/anatomy_version_stamp.hpp direkt ein und folgt damit dem A14-Stempel-Guard (Boost::mp11 + builder).
add_executable(test_od10_numa_page_probe "test_od10_numa_page_probe.cpp")
target_compile_features(test_od10_numa_page_probe PRIVATE cxx_std_23)
target_include_directories(test_od10_numa_page_probe PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(test_od10_numa_page_probe PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_od10_numa_page_probe)
target_compile_definitions(test_od10_numa_page_probe PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_od10_numa_page_probe COMMAND test_od10_numa_page_probe)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_od10_numa_page_probe)
set_tests_properties(test_od10_numa_page_probe PROPERTIES LABELS "numapageprobe;measurement")

# Paket OD-11-RT: prozessfreie RT-Erhebung der target_isa-Unter-Achse core_class (Ausfuehrungs-
# Lokalitaet) plus der Zuordnungs-Faehigkeit, per CT-Familien-Spezialisierung -- das Pendant zu
# OD-10-RT, das die Speicher-Lokalitaet traegt. Der Test faehrt die tragende Klassifikations-Logik
# HARDWARE-FREI gegen eingespeiste Topologie-Fassungen (hybrid / uniform / L3-asymmetrisch), belegt
# jede K4-Fehlerklasse einzeln, liest die Live-Maschine mit benannten Skip-Guards und prueft die
# A-15-Stempel-Neutralitaet. Include-/Link-Menge identisch zu test_od10_numa_page_probe: auch diese TU
# bindet abi/anatomy_version_stamp.hpp direkt ein und folgt damit dem A14-Stempel-Guard.
add_executable(test_od11_numa_cpu_pin_process_probe "test_od11_numa_cpu_pin_process_probe.cpp")
target_compile_features(test_od11_numa_cpu_pin_process_probe PRIVATE cxx_std_23)
target_include_directories(test_od11_numa_cpu_pin_process_probe PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(test_od11_numa_cpu_pin_process_probe PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_od11_numa_cpu_pin_process_probe)
target_compile_definitions(test_od11_numa_cpu_pin_process_probe PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_od11_numa_cpu_pin_process_probe COMMAND test_od11_numa_cpu_pin_process_probe)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_od11_numa_cpu_pin_process_probe)
set_tests_properties(test_od11_numa_cpu_pin_process_probe PROPERTIES LABELS "numacpupinprocessprobe;measurement")

# Task #7 P3: der Stufe-1-REAL-Beweis (F13 "erst prod1 testen"). Fixture-Teil: v1-Kontrakt-Dateien in
# der ECHTEN Schreiber-Form (Werte-Schluessel je DIMM wiederholt -- vom live verifizierten Format
# abgeleitet, A9); Live-Teil: liest den echten Boot-Cache dieses Hosts mit benanntem Skip-Guard
# (CI-Docker hat keinen Infra-Schreiber). Die Live-Erwartung wird aus der Datei selbst unabhaengig
# zweitgelesen, nie als Zahl gepinnt (Owner-KERN: Hardware-Werte nie statisch). Include-Menge wie P2.
foreach(_p3_test test_p3_boot_cache_dmi)
    add_executable(${_p3_test} "${_p3_test}.cpp")
    target_compile_features(${_p3_test} PRIVATE cxx_std_23)
    target_include_directories(${_p3_test} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated")
    target_link_libraries(${_p3_test} PRIVATE gtest gtest_main)
    comdare_set_platform_defines(${_p3_test})
    target_compile_definitions(${_p3_test} PRIVATE
        COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
    add_test(NAME ${_p3_test} COMMAND ${_p3_test})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_p3_test})
    set_tests_properties(${_p3_test} PROPERTIES LABELS "hwprobe;measurement")
endforeach()

# O-4 / C-3a: die zwei IDENTITAETS-Tests. Sie liefen bis V-4 nur per Hand-Bau -- der einzige in den
# Dateikoepfen genannte Grund war Paket-Disziplin ("tests/unit/CMakeLists.txt bleibt unangetastet"),
# kein inhaltlicher. Ein Identitaets-Test ohne Gate ist ein unsichtbarer Test: er faengt eine
# Regression erst, wenn jemand ihn zufaellig von Hand baut. Beide tragen dieselbe leichte
# Include-Menge wie test_p1 (kein Boost::mp11, keine topics/axis.hpp-Kette).
foreach(_ident_test test_o4_machine_identity test_c3a_gate_scharfschaltung)
    add_executable(${_ident_test} "${_ident_test}.cpp")
    target_compile_features(${_ident_test} PRIVATE cxx_std_23)
    target_include_directories(${_ident_test} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated")
    target_link_libraries(${_ident_test} PRIVATE gtest gtest_main)
    comdare_set_platform_defines(${_ident_test})
    target_compile_definitions(${_ident_test} PRIVATE
        COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
    add_test(NAME ${_ident_test} COMMAND ${_ident_test})
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${_ident_test})
    set_tests_properties(${_ident_test} PROPERTIES LABELS "identitaet;measurement")
endforeach()

# MT-L4 (09.08.2026): C-3b nachregistriert -- die SCHWESTERSTELLE, die beim Wiring von C-3a liegen blieb.
# Der Block darueber holte C-3a mit der Begruendung "ein Identitaets-Test ohne Gate ist ein unsichtbarer
# Test" in den Bauweg; C-3b prueft DIESELBE Header-Familie (simd_build_gate/simd_organ_requirement/
# simd_organ_sensibility/simd_sub_axis/machine_simd_signature) und blieb dabei aussen vor. Am Objekt
# gemessen (09.08.2026): 0 Treffer in dieser Datei, 0 Zeilen in build.ninja -- seit 26.07. nie gebaut.
# Gleicher Bau-Zuschnitt wie der Nachbar, denn es ist derselbe Include-Kegel; anders als C-3a ist C-3b
# ein ECHTER gtest (8 TEST()-Faelle), gtest/gtest_main werden also wirklich gezogen.
# Eigenes Label statt "identitaet": C-3b belegt nicht die Maschinen-IDENTITAET, sondern dass Baseline-march
# und Gate-Extras auf DERSELBEN Compile-Zeile zusammenlaufen (Bauplan-v3 D2.2) -- ein Beleg-Paket, das
# nichts aendert und die Vorbedingung jeder Scharfschaltung ist.
add_executable(test_c3b_kanal_merge_beleg "test_c3b_kanal_merge_beleg.cpp")
target_compile_features(test_c3b_kanal_merge_beleg PRIVATE cxx_std_23)
target_include_directories(test_c3b_kanal_merge_beleg PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(test_c3b_kanal_merge_beleg PRIVATE gtest gtest_main)
comdare_set_platform_defines(test_c3b_kanal_merge_beleg)
target_compile_definitions(test_c3b_kanal_merge_beleg PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_c3b_kanal_merge_beleg COMMAND test_c3b_kanal_merge_beleg)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_c3b_kanal_merge_beleg)
set_tests_properties(test_c3b_kanal_merge_beleg PROPERTIES LABELS "simd;kanal_merge;measurement" TIMEOUT 60)

# Querschnitt M - W12-A (§43): X.Y.Z-Parse-Helfer + Planer-Selbst-Stempel (leichte TU, keine Registries).
add_executable(test_m_w12_stamp_bausteine "test_m_w12_stamp_bausteine.cpp")
target_compile_features(test_m_w12_stamp_bausteine PRIVATE cxx_std_23)
target_include_directories(test_m_w12_stamp_bausteine PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
# Hermetik (CI-3): planner_version.hpp zieht transitiv topics/axis.hpp (boost::mp11) -- explizit linken.
# M-1/D-4: comdare_measurement_combo_ct ist die EINE Emissionsstelle der einkompilierten Mess-Combo. Diese
# TU zieht builder/ceb_version_stamp.hpp; ohne den Link braeche sie mit dem benannten #error der
# Verdrahtungs-Wache (und haette vorher, still, einen anderen kCebFingerprint gesehen als der Fassaden-Link).
target_link_libraries(test_m_w12_stamp_bausteine PRIVATE gtest gtest_main Boost::mp11
    comdare_measurement_combo_ct)
comdare_set_platform_defines(test_m_w12_stamp_bausteine)
target_compile_definitions(test_m_w12_stamp_bausteine PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_m_w12_stamp_bausteine COMMAND test_m_w12_stamp_bausteine)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m_w12_stamp_bausteine)
set_tests_properties(test_m_w12_stamp_bausteine PROPERTIES LABELS "m;blut")

# -- M-1/D-4 (06.08.2026): DER BISS des CEB-Schluessels -----------------------------------------------
# Weist beidseitig nach, dass kCebFingerprint an der EINKOMPILIERTEN Mess-Combo haengt: verschiedene Combo
# => verschiedener Schluessel (21 Paare ueber alle 7 nicht-leeren Tooling-Teilmengen), gleiche Combo =>
# derselbe Schluessel ueber einen ZWEITEN, unabhaengigen (Laufzeit-)Ableitungsweg. Dazu der Byte-Anker der
# Vollmenge, der den Alt-Bestand an ceb_key_sha512 vor stiller Entwertung schuetzt.
# Include-/Link-Menge == test_m_w12_stamp_bausteine (dieselbe Stempel-Flaeche).
add_executable(test_d4_ceb_schluessel_wahl "test_d4_ceb_schluessel_wahl.cpp")
target_compile_features(test_d4_ceb_schluessel_wahl PRIVATE cxx_std_23)
target_include_directories(test_d4_ceb_schluessel_wahl PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_d4_ceb_schluessel_wahl PRIVATE gtest gtest_main Boost::mp11
    comdare_measurement_combo_ct)
comdare_set_platform_defines(test_d4_ceb_schluessel_wahl)
target_compile_definitions(test_d4_ceb_schluessel_wahl PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_d4_ceb_schluessel_wahl COMMAND test_d4_ceb_schluessel_wahl)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_d4_ceb_schluessel_wahl)
set_tests_properties(test_d4_ceb_schluessel_wahl PROPERTIES LABELS "m;d4;stempel;fehlerklassen")

# -- M-1/D-2 (06.08.2026): DER BISS des Mess-Vertrags CEB <-> Tier-Binary ------------------------------
# Weist nach, dass eine Tier-Binary mit FALSCHER (oder fehlender) Mess-Deklaration am Pruefdock abgewiesen
# wird. Drei Ebenen, und jede traegt etwas Eigenes: (A) die ECHTE Makro-Expansion in der TU selbst
# (COMDARE_ANATOMY_VERSION_STAMP_M, wie in test_w10_system_cell_values), (B) eine REALE gebaute .so ueber
# die dlopen-Grenze (r5g-adhoc-Fixture -- sie besteht das FUNKTIONS-Gate und faellt am MESS-Vertrag, weil
# sie gar keinen Stempel traegt), (C) manipulierte PODs, deren Entry-Arrays aus DEMSELBEN consteval-Parser
# stammen wie die des Makros.
# LINK-MENGE: Stempel-Flaeche (wie test_m_w12) PLUS comdare::anatomy_module_loader fuer Ebene B (dieselbe
# Menge wie test_pruef_only_gate, das dieselbe Fixture faehrt).
add_executable(test_d2_mess_konsistenz_gate "test_d2_mess_konsistenz_gate.cpp")
target_compile_features(test_d2_mess_konsistenz_gate PRIVATE cxx_std_23)
target_include_directories(test_d2_mess_konsistenz_gate PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_d2_mess_konsistenz_gate PRIVATE gtest gtest_main Boost::mp11
    comdare::anatomy_module_loader)
comdare_set_platform_defines(test_d2_mess_konsistenz_gate)
target_compile_definitions(test_d2_mess_konsistenz_gate PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1
    "COMDARE_R5G_ADHOC_DLL_DIR=\"${_r5g_adhoc_dir}\"")
add_dependencies(test_d2_mess_konsistenz_gate comdare_r5g_adhoc_perm)
add_test(NAME test_d2_mess_konsistenz_gate COMMAND test_d2_mess_konsistenz_gate)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_d2_mess_konsistenz_gate)
set_tests_properties(test_d2_mess_konsistenz_gate PROPERTIES LABELS "m;d2;stempel;pruefdock;fehlerklassen")

# W10-C1 (Bauplan-Dossier 20260803): die Single-Source der System-ZELLWERTE (abi/system_cell_values.hpp).
# Orakel: Ziel-Zeilen-Form, je Fehlform eine benannte CT-Diagnose, Identitaet bei leerem Werte-Set,
# Grammatik-Haertung der vervollstaendigten Zeile und die Drift-Wachen gegen kSystemAxisOrder UND
# ExternalUtilsHub::meta_metas. Include-/Link-Menge == test_m_w12_stamp_bausteine (dieselbe Stempel-
# Bausteine-Familie): Boost::mp11, weil die TU die Meta-Meta-Hub-Typen fuer die Drift-Wache zieht, und
# der builder-/src-Pfad, weil der C3-Zwillings-Beweis derselben TU ueber bestandslog/ctsha512 laeuft.
add_executable(test_w10_system_cell_values "test_w10_system_cell_values.cpp")
target_compile_features(test_w10_system_cell_values PRIVATE cxx_std_23)
target_include_directories(test_w10_system_cell_values PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_w10_system_cell_values PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_w10_system_cell_values)
target_compile_definitions(test_w10_system_cell_values PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_w10_system_cell_values COMMAND test_w10_system_cell_values)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_w10_system_cell_values)
set_tests_properties(test_w10_system_cell_values PROPERTIES LABELS "w10;stempel;blut")

# K7b-4 (§62-B, G1/B6-Auflage): G1-Je-Binary-Selbst-Stempel-Header (Format-Pin + non-empty je Feld; leichte TU).
add_executable(test_g1_binary_version_stamp "test_g1_binary_version_stamp.cpp")
target_compile_features(test_g1_binary_version_stamp PRIVATE cxx_std_23)
target_include_directories(test_g1_binary_version_stamp PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
# Hermetik (CI-3): planner_version.hpp zieht transitiv topics/axis.hpp (boost::mp11) -- explizit linken.
target_link_libraries(test_g1_binary_version_stamp PRIVATE gtest gtest_main Boost::mp11)
comdare_set_platform_defines(test_g1_binary_version_stamp)
target_compile_definitions(test_g1_binary_version_stamp PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_g1_binary_version_stamp COMMAND test_g1_binary_version_stamp)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_g1_binary_version_stamp)
set_tests_properties(test_g1_binary_version_stamp PROPERTIES LABELS "m;blut")

# S1 (§62-B Log-Flush): ProgressHeartbeat -- geflushtes zeit-gatetes Fortschritts-Testat (leichte TU, nur std).
add_executable(test_progress_heartbeat "test_progress_heartbeat.cpp")
target_compile_features(test_progress_heartbeat PRIVATE cxx_std_23)
target_include_directories(test_progress_heartbeat PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")
target_link_libraries(test_progress_heartbeat PRIVATE gtest gtest_main)
comdare_set_platform_defines(test_progress_heartbeat)
add_test(NAME test_progress_heartbeat COMMAND test_progress_heartbeat)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_progress_heartbeat)
set_tests_properties(test_progress_heartbeat PROPERTIES LABELS "m;blut")

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
# NEW-GOLDEN-ALL-AXES: der 2^17-Anker ist die committete CRC64 (kNewGolden131072Crc64, source_catalog.hpp), NICHT eine
# 62-MB-Datei — der Test regeneriert die 131072 ids lazy und prueft ihre CRC64. Keine golden-131072-Datei im git.
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_limits_entkopplung_vorstufe)
set_tests_properties(test_limits_entkopplung_vorstufe PROPERTIES LABELS "e4;limits")

# INC-G6 (Ledger 33/34/35): Round-Trip-Gate des lazy Per-Index-Adhoc-Emitters (lazy_adhoc_source_gen.hpp).
# Belegt VOR einem golden-N-Voll-Lauf, dass der lazy Gen (a) fuer die 320er-ids byte-identisch zum Katalog-Gen
# rendert, (b) fuer Nicht-320 golden-N-ids nicht-leer + wert-korrekt liefert und (c) die StaticBinaryView-ids
# den committeten CRC64-Anker treffen. Gleiche Abhaengigkeiten wie test_limits (generierter 320er-Katalog +
# source_catalog + experiment_tree + crc64) -> spiegelt dessen Registrierung inkl. comdare_attach_generated_catalog.
add_executable(test_lazy_adhoc_source_gen
    "test_lazy_adhoc_source_gen.cpp")
target_compile_features(test_lazy_adhoc_source_gen PRIVATE cxx_std_23)
target_include_directories(test_lazy_adhoc_source_gen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_lazy_adhoc_source_gen PRIVATE Boost::mp11)
comdare_set_platform_defines(test_lazy_adhoc_source_gen)
target_compile_definitions(test_lazy_adhoc_source_gen PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
comdare_attach_generated_catalog(test_lazy_adhoc_source_gen)
add_test(NAME test_lazy_adhoc_source_gen COMMAND test_lazy_adhoc_source_gen)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_lazy_adhoc_source_gen)
set_tests_properties(test_lazy_adhoc_source_gen PROPERTIES LABELS "e4;limits;inc-g6")

# -- LAG-Z1 (2026-08-09): DER LAGER-SCHLUESSEL MUSS DIE SIMD-STUFE TRENNEN -----------------------------
# SELBSTCHECK dieses Blocks: der Include-/Define-/Katalog-Satz ist WORTGLEICH zu test_lazy_adhoc_source_gen
# direkt darueber -- dieselbe Naht (lazy_adhoc_source_gen.hpp), dieselbe umbrella-schwere Katalog-Kante.
# EINZIGE Ergaenzung: libs/cache_engine/profile_facade, weil dieses Ziel zusaetzlich die beiden Perm-Naehte
# zieht (system_cell_values_naht.hpp, toolchain_stamp_naht.hpp) -- genau die Quellen, aus denen
# profile_run_entry.hpp:1008-1029 die Perm-Zelle bildet. Ohne sie waere die Wache eine ZWEITE Ableitung.
# comdare_attach_generated_catalog ist Pflicht (P1689-Scan vs. Generator-Custom-Command, s. der Kommentar
# am test_experiment_plan_director-Block).
add_executable(test_lagz1_lager_schluessel_simd
    "test_lagz1_lager_schluessel_simd.cpp")
target_compile_features(test_lagz1_lager_schluessel_simd PRIVATE cxx_std_23)
target_include_directories(test_lagz1_lager_schluessel_simd PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_lagz1_lager_schluessel_simd PRIVATE Boost::mp11)
comdare_set_platform_defines(test_lagz1_lager_schluessel_simd)
target_compile_definitions(test_lagz1_lager_schluessel_simd PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
comdare_attach_generated_catalog(test_lagz1_lager_schluessel_simd)
add_test(NAME test_lagz1_lager_schluessel_simd COMMAND test_lagz1_lager_schluessel_simd)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_lagz1_lager_schluessel_simd)
set_tests_properties(test_lagz1_lager_schluessel_simd PROPERTIES LABELS "lager;bestandslog;lag-z1")
# DER BISS FAEHRT MIT. Eine Wache, die nur am gesunden Objekt gruen ist, belegt nichts. --selbstbiss
# ersetzt WEG L durch den lauf-konstanten Provider -- exakt den historischen Rueckfall, den
# profile_run_entry.hpp:1074-1082 beschreibt -- und FORDERT, dass GENAU die Trennungs-Faelle reissen
# und die SKIP-/Determinismus-Faelle halten. Zweiter ctest-Eintrag auf DERSELBEN Binary: kein zweites
# Ziel, kein zweiter Bau, aber der Nenner faehrt bei jedem Lauf mit statt hier als Prosa zu verjaehren.
add_test(NAME test_lagz1_lager_schluessel_simd_selbstbiss
    COMMAND test_lagz1_lager_schluessel_simd --selbstbiss)
set_tests_properties(test_lagz1_lager_schluessel_simd_selbstbiss
    PROPERTIES LABELS "lager;bestandslog;lag-z1;selbstbiss")

# W2 (Owner-GO mittag-6 R1, 2026-08-05) -- Flaechen-Gate des CT-Stempel-VORRANGS an der EINEN Stempel-Naht
# measurement_stamp_from_env. DIESE TU ist der einzige Ort im Baum, der MIT der einkompilierten Mess-Combo
# uebersetzt wird (COMDARE_MEASUREMENT_COMBO_CT) -- alle anderen TUs (auch test_lazy_adhoc_source_gen direkt
# darueber) kompilieren weiter den #else-Env-Zweig und belegen dessen unveraenderte Semantik. Eigene Binary =>
# kein ODR-Konflikt zwischen den beiden Auspraegungen der inline-Funktion.
# Include-Satz + Katalog-Anhang wie test_lazy_adhoc_source_gen (die Naht wohnt in lazy_adhoc_source_gen.hpp).
add_executable(test_w2_combo_ct_stamp
    "test_w2_combo_ct_stamp.cpp")
target_compile_features(test_w2_combo_ct_stamp PRIVATE cxx_std_23)
target_include_directories(test_w2_combo_ct_stamp PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_w2_combo_ct_stamp PRIVATE Boost::mp11)
comdare_set_platform_defines(test_w2_combo_ct_stamp)
target_compile_definitions(test_w2_combo_ct_stamp PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    "COMDARE_MEASUREMENT_COMBO_CT=\"[wallclock]\"")
comdare_attach_generated_catalog(test_w2_combo_ct_stamp)
add_test(NAME test_w2_combo_ct_stamp COMMAND test_w2_combo_ct_stamp)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_w2_combo_ct_stamp)
set_tests_properties(test_w2_combo_ct_stamp PROPERTIES LABELS "w2;stempel;fehlerklassen")

# M-1/D-1 BISS (06.08.2026): die Mess-Achse WIRKT auf die Tier-Defines. Prueft die reine Abbildung
# profile_facade/mess_achsen_naht.hpp mess_achsen_defines() -- die EINE Funktion, aus der
# perm_mess_defines() (profile_run_facade.cpp) ihren Mess-Teil zieht.
# BEWUSST OHNE COMDARE_MEASUREMENT_COMBO_CT: der Biss beisst an der ABBILDUNG (Legende -> Defines),
# nicht am Makro-Zustand dieser TU. Er muss unabhaengig davon rot werden, wie die CEB gerade
# konfiguriert ist -- sonst prueft er die Umgebung statt die Aenderung.
add_executable(test_m1_mess_achsen_wirkung
    "test_m1_mess_achsen_wirkung.cpp")
target_compile_features(test_m1_mess_achsen_wirkung PRIVATE cxx_std_23)
target_include_directories(test_m1_mess_achsen_wirkung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}")
comdare_set_platform_defines(test_m1_mess_achsen_wirkung)
add_test(NAME test_m1_mess_achsen_wirkung COMMAND test_m1_mess_achsen_wirkung)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m1_mess_achsen_wirkung)
set_tests_properties(test_m1_mess_achsen_wirkung PROPERTIES LABELS "m1;messachse;stempel;fehlerklassen")

# -- R-3 BISS (07.08.2026): DER MESS-GATE-ZUSTAND DER TU DISKRIMINIERT DEN TIER-FINGERPRINT ------------
# Zwei .so aus EINEM Quelltext (r3_mess_gate_stamp_module.cpp) mit byte-identischen Stempel-Literalen;
# der einzige Unterschied ist der Praeprozessor-Zustand der Mess-Gates. Am Vor-R-3-Stand liefern beide
# denselben sha512_line (kein Gate-Makro im Preimage) -> der Runner ist ROT. Mit dem neunten
# Preimage-Glied "mess-gates" sind sie verschieden -> GRUEN.
# Praezedenz der Modul-TU-Targets: die perm_*-DLLs der Goal-V6-Schleife oben; Include-/Link-Menge der
# Stempel-Flaeche: test_w10_system_cell_values (dieselbe Makro-Naht COMDARE_ANATOMY_VERSION_STAMP_M).
# GATES-AUS wird per #undef IN DER QUELLE praepariert (Praezedenz test_a8s4_release_pfad_neutralitaet):
# die Wurzel-CMakeLists setzt COMDARE_MEASUREMENT_ON=1 per add_compile_definitions GLOBAL, ein Target
# kann eine Verzeichnis-Definition nicht abwaehlen.
set(_r3_biss_dll_dir "${PROJECT_BINARY_DIR}/generated/r3_biss_modules")
foreach(_r3mod IN ITEMS r3_biss_modul_gates_an r3_biss_modul_gates_aus)
    add_library(${_r3mod} SHARED "r3_mess_gate_stamp_module.cpp")
    target_compile_features(${_r3mod} PRIVATE cxx_std_23)
    target_include_directories(${_r3mod} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_r3mod} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_r3mod})
    set_target_properties(${_r3mod} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        LIBRARY_OUTPUT_DIRECTORY  "${_r3_biss_dll_dir}"
        RUNTIME_OUTPUT_DIRECTORY  "${_r3_biss_dll_dir}")
    # Hermetik: die Modul-TU zieht ueber abi_adapter.hpp die is_original-Codegen-Header. Die
    # Codegen-Kante kann hier nicht direkt gesetzt werden (comdare_paper_codegen_all entsteht erst
    # NACH add_subdirectory(tests)); die Wurzel-CMakeLists zieht sie ueber diese Registry nach.
    # KEIN Test-Target -> die COMDARE_TEST_TARGETS-Schleife der Wurzel erreicht sie nicht.
    set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS ${_r3mod})
endforeach()
target_compile_definitions(r3_biss_modul_gates_an PRIVATE
    COMDARE_ANATOMY_MODULE_BUILD=1 COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1
    COMDARE_EXPERIMENT_MODE_ON=1 COMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1)
target_compile_definitions(r3_biss_modul_gates_aus PRIVATE
    COMDARE_ANATOMY_MODULE_BUILD=1 COMDARE_R3_BISS_GATES_AUS=1)

add_executable(test_r3_mess_gate_fingerprint_biss "test_r3_mess_gate_fingerprint_biss.cpp")
target_compile_features(test_r3_mess_gate_fingerprint_biss PRIVATE cxx_std_23)
target_include_directories(test_r3_mess_gate_fingerprint_biss PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_r3_mess_gate_fingerprint_biss PRIVATE ${CMAKE_DL_LIBS})
comdare_set_platform_defines(test_r3_mess_gate_fingerprint_biss)
add_dependencies(test_r3_mess_gate_fingerprint_biss r3_biss_modul_gates_an r3_biss_modul_gates_aus)
add_test(NAME test_r3_mess_gate_fingerprint_biss COMMAND test_r3_mess_gate_fingerprint_biss
    "--an=$<TARGET_FILE:r3_biss_modul_gates_an>" "--aus=$<TARGET_FILE:r3_biss_modul_gates_aus>")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_r3_mess_gate_fingerprint_biss)
set_tests_properties(test_r3_mess_gate_fingerprint_biss PROPERTIES
    LABELS "r3;messachse;stempel;dll_roundtrip")

# R-3 SPIEGEL-WACHE: die HOST-Vorhersage des neunten Glieds (mess_gates_glied_for_legend) gegen die
# TU-Wahrheit (abi::kMessGatesTuGlied) -- und die Host-Nachrechnung der beiden real gebauten .so.
# Include-/Link-Menge: die Mess-Naht (wie test_m1_mess_achsen_wirkung) PLUS der src/-Pfad fuer die
# SHA-512-Primitive und ${CMAKE_DL_LIBS} fuer die dlopen-Haelfte.
add_executable(test_r3_mess_gates_spiegel "test_r3_mess_gates_spiegel.cpp")
target_compile_features(test_r3_mess_gates_spiegel PRIVATE cxx_std_23)
target_include_directories(test_r3_mess_gates_spiegel PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_r3_mess_gates_spiegel PRIVATE ${CMAKE_DL_LIBS})
comdare_set_platform_defines(test_r3_mess_gates_spiegel)
target_compile_definitions(test_r3_mess_gates_spiegel PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_dependencies(test_r3_mess_gates_spiegel r3_biss_modul_gates_an r3_biss_modul_gates_aus)
add_test(NAME test_r3_mess_gates_spiegel COMMAND test_r3_mess_gates_spiegel
    "--an=$<TARGET_FILE:r3_biss_modul_gates_an>" "--aus=$<TARGET_FILE:r3_biss_modul_gates_aus>")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_r3_mess_gates_spiegel)
set_tests_properties(test_r3_mess_gates_spiegel PROPERTIES
    LABELS "r3;messachse;stempel;dll_roundtrip")

# M-1/H-A + M-1/H-2 (06.08.2026): die STUFEN-Wache im CT-losen Zweig und die PMC-Ausstattungs-Invariante.
# BEWUSST OHNE COMDARE_MEASUREMENT_COMBO_CT -- und hier ist das nicht bloss Unabhaengigkeit vom Makro-
# Zustand wie beim Nachbarn oben, sondern die SACHE selbst: die H-A-Luecke SASS im #else-Zweig. Eine TU,
# die comdare_measurement_combo_ct linkt, wuerde den geheilten Defekt gar nicht erreichen. Fall (a)
# verifiziert diese Voraussetzung zur Laufzeit, damit die Zusage nicht bloss hier im CMake steht.
# B-5 (08.08.2026): PMC-Ehrlichkeit fuer ALLE SIEBEN Zaehler + errno-Klassentrennung.
# KEIN 'pmc'-Label -- und das ist eine Lehre aus M-1/M-CE-25, keine Nachlaessigkeit: diese TU oeffnet
# KEINEN Counter und liest KEINE PMU, sie prueft die reine Renderer-/Klassifikations-Logik mit von Hand
# gesetzten POD-Feldern. Mit dem Label schloesse test:unit (-LE pmc) sie aus, waehrend die .pmc-Jobs nur
# eine explizite --target-Liste bauen -- 'Not Run' in jedem Job, nie wirklich geprueft.
add_executable(test_b5_pmc_ehrlichkeit_alle_sieben "test_b5_pmc_ehrlichkeit_alle_sieben.cpp")
target_compile_features(test_b5_pmc_ehrlichkeit_alle_sieben PRIVATE cxx_std_23)
target_include_directories(test_b5_pmc_ehrlichkeit_alle_sieben PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_b5_pmc_ehrlichkeit_alle_sieben PRIVATE Boost::mp11)
comdare_set_platform_defines(test_b5_pmc_ehrlichkeit_alle_sieben)
target_compile_definitions(test_b5_pmc_ehrlichkeit_alle_sieben PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_b5_pmc_ehrlichkeit_alle_sieben COMMAND test_b5_pmc_ehrlichkeit_alle_sieben)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_b5_pmc_ehrlichkeit_alle_sieben)
set_tests_properties(test_b5_pmc_ehrlichkeit_alle_sieben PROPERTIES LABELS "b5;csv;standalone")

add_executable(test_m1h_stufen_und_pmc_wache
    "test_m1h_stufen_und_pmc_wache.cpp")
target_compile_features(test_m1h_stufen_und_pmc_wache PRIVATE cxx_std_23)
target_include_directories(test_m1h_stufen_und_pmc_wache PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_CURRENT_SOURCE_DIR}")
comdare_set_platform_defines(test_m1h_stufen_und_pmc_wache)
add_test(NAME test_m1h_stufen_und_pmc_wache COMMAND test_m1h_stufen_und_pmc_wache)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m1h_stufen_und_pmc_wache)
# KORREKTUR (2026-08-07, CI-Heilung nach M-1): das Label 'pmc' entfernt. Der Test prueft REINE
# CT-/Konfigurationslogik (pruefe_pmc_ausstattung/resolve_live_measurement_combo_legend als benannte
# Funktionen mit explizit uebergebenen bool-Parametern) -- NICHT die Laufzeit-Verfuegbarkeit echter
# PMC-Hardware. Fall (d) durchlaeuft micro/pmc=true/false-Kombinationen ALS PARAMETER, unabhaengig vom
# eigenen COMDARE_ENABLE_PMC-Build-Zustand dieser TU; Fall (e) ist im CT-losen Zustand dieser TU (kein
# comdare_measurement_combo_ct verlinkt) in JEDEM Fall ein No-op. Das Ergebnis ist also identisch, ob
# diese Binary mit COMDARE_ENABLE_PMC=ON oder =OFF gebaut wird -- keine Hardware-Abhaengigkeit.
# Mit dem Label 'pmc' schloss test:unit (-LE pmc) den Test aus, waehrend der .pmc-Job (Template
# .bare_metal) nur eine explizite --target-Liste baut, die dieser Test nie erreichte -- 'Not Run' in
# pmc:amd/pmc:intel, nie in irgendeinem Job wirklich geprueft. DIESELBE Fehlerklasse wie M-CE-25/
# Muster-F (2026-07-13, m3v2_pmc_smoke), von der R4-Abdeckungswache diesmal LAUT statt still gemacht --
# die Wache hat ihren Zweck erfuellt. Ohne das Label faellt der Test unter test:unit (baut comdare_tests
# vollstaendig) und wird dort real ausgefuehrt.
set_tests_properties(test_m1h_stufen_und_pmc_wache PROPERTIES LABELS "m1;messachse;stufen;fehlerklassen")

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
    comdare_add_simd_dispatch_test("_avx2" "-mavx2" "${COMDARE_HOST_RUNS_AVX2}")
else()
    # Fehlerklassifizierung INC-29.2 (D1): HW-/Compile-Erweiterungs-Fehlen ist ein ERKENNBARER,
    # klassifizierbarer Zustand -- KEIN stiller Skip. Der Experiment-Planer (Compiler-Compiler) deklariert
    # ihn IM LOG und faehrt weiter (die Suite baut ohne diese Variante). Feuert nur auf Hosts, deren Compiler
    # -mavx2 nicht kennt (nicht auf den CI-Prod-Hosts -> CI-neutral).
    message(WARNING "[Compiler-Compiler-Fehler: hardware_erweiterung_fehlt] Compiler kennt -mavx2 nicht -> "
                    "avx2-SIMD-Dispatch-Variante uebersprungen. Klassifiziert+geloggt, kein stiller Skip, kein Absturz.")
endif()

# AVX-512F-Variante: bauen, wenn der Compiler -mavx512f kennt; ausfuehren nur bei Host-AVX-512F.
check_cxx_compiler_flag("-mavx512f" COMDARE_CXX_HAS_MAVX512F)
if(COMDARE_CXX_HAS_MAVX512F)
    comdare_add_simd_dispatch_test("_avx512" "-mavx512f" "${COMDARE_HOST_RUNS_AVX512F}")
else()
    # Fehlerklassifizierung INC-29.2 (D1), analog avx2: -mavx512f-Fehlen klassifiziert+geloggt, Suite baut weiter.
    message(WARNING "[Compiler-Compiler-Fehler: hardware_erweiterung_fehlt] Compiler kennt -mavx512f nicht -> "
                    "avx512-SIMD-Dispatch-Variante uebersprungen. Klassifiziert+geloggt, kein stiller Skip, kein Absturz.")
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
    comdare_add_simd_add_u64_test("_avx2" "-mavx2" "${COMDARE_HOST_RUNS_AVX2}")
endif()
if(COMDARE_CXX_HAS_MAVX512F)
    comdare_add_simd_add_u64_test("_avx512" "-mavx512f" "${COMDARE_HOST_RUNS_AVX512F}")
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
# WP-4/F29+F64(a) (Voll-Audit 2026-07-16) — Registry-Round-Trip-Gate (Muster R-E/M-CE-27):
# die committete cache_engine_axis_registry.xml deklariert "Reflektion ueber Enabled* ist Pflicht
# (Round-Trip-Garantie)", aber der Generator (comdare_axis_registry_gen, EXCLUDE_FROM_ALL) lief in
# KEINEM ctest/CI-Job — Enabled*-Aenderungen driften stumm an der XML vorbei (M-CE-27-Stale-Klasse).
# Script-Test (CMake-Primitiv, KEIN gtest): registry_roundtrip.cmake laesst das VOR-gebaute Tool
# nach CMAKE_CURRENT_BINARY_DIR regenerieren + byte-difft gegen die committete XML (NUR GELESEN).
# Referenz-Konfiguration: Default-Standalone (s. Script-Kopf). Das +contract-Label haelt den Test
# aus test:unit (-LE contract|pmc) heraus — CI-Heimat ist contract:profile_coverage (Registry-/
# Profil-Wahrheit), dort Target in der Build-Liste + Name im -R-Regex (Doktrin: GENAU EIN Job).
# COMDARE_TEST_TARGETS-Registrierung zieht das EXCLUDE_FROM_ALL-Tool in den comdare_tests-Sammel-Bau
# (Registry-Doktrin Root-CMakeLists Z.1505: jedes ctest-registrierte Target wird gebaut).
# ─────────────────────────────────────────────────────────────────────────────
if(TARGET comdare_axis_registry_gen)
    add_test(
        NAME test_axis_registry_roundtrip
        COMMAND "${CMAKE_COMMAND}"
            "-DGEN_TOOL=$<TARGET_FILE:comdare_axis_registry_gen>"
            "-DCOMMITTED=${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml"
            "-DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/registry_roundtrip_ce"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/registry_roundtrip.cmake")
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS comdare_axis_registry_gen)
    set_tests_properties(test_axis_registry_roundtrip
        PROPERTIES LABELS "contract;registry;roundtrip" TIMEOUT 120)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# PAKET W2-B (2026-07-19) — die 2 FEHLENDEN Art-Registry-Roundtrip-Gates (Ledger §28/§30). Muster wie
# test_axis_registry_roundtrip (registry_roundtrip.cmake WIEDERVERWENDET): das VOR-gebaute Generator-Tool
# regeneriert nach WORKDIR + byte-difft gegen die committete XML (NUR GELESEN). DIESE Roundtrip-Tests SIND
# der geforderte Konsument+Contract-Test IM SELBEN Increment (axis_binding-Anti-Lehre: keine Registry ohne
# verdrahteten Konsumenten). ABWEICHUNG zum Vorbild — KEIN contract-Label: gemaess Kapseln-Lehre C.1/L2
# MUESSEN diese Gates im DEFAULT-Sweep test:unit (-LE contract|pmc) laufen, damit Drift LOKAL sofort faengt
# (das Vorbild ist nur wegen seiner schweren registry_to_axis_levels-TU nach contract:profile_coverage
# ausgelagert; diese 2 Generatoren sind leichte Mess-/System-TUs ohne Organ-Topic-Registries).
#
# EXISTS-GUARD (bindend): das committete XML existiert ERST, nachdem der Architekt nach dem seriellen Build
# das Tool laufen liess + die XML committete (Tool-Bau ist in W2-B gesperrt, Platzhalter verboten). Solange
# die XML fehlt, wird das Gate NICHT registriert (Pipeline bleibt hart gruen); es bewaffnet sich beim naechsten
# Configure automatisch, sobald die generierte XML committet ist. Erst-Generierungs-Befehl siehe W2-B-Report.
# ─────────────────────────────────────────────────────────────────────────────
set(_w2b_system_registry
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement/system_axis_registry.xml")
if(TARGET comdare_system_axis_registry_gen AND EXISTS "${_w2b_system_registry}")
    add_test(
        NAME test_system_axis_registry_roundtrip
        COMMAND "${CMAKE_COMMAND}"
            "-DGEN_TOOL=$<TARGET_FILE:comdare_system_axis_registry_gen>"
            "-DCOMMITTED=${_w2b_system_registry}"
            "-DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/registry_roundtrip_system"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/registry_roundtrip.cmake")
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS comdare_system_axis_registry_gen)
    set_tests_properties(test_system_axis_registry_roundtrip
        PROPERTIES LABELS "registry;roundtrip;system" TIMEOUT 120)
endif()

set(_w2b_measurement_registry
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement/measurement_axis_registry.xml")
if(TARGET comdare_measurement_axis_registry_gen AND EXISTS "${_w2b_measurement_registry}")
    add_test(
        NAME test_measurement_axis_registry_roundtrip
        COMMAND "${CMAKE_COMMAND}"
            "-DGEN_TOOL=$<TARGET_FILE:comdare_measurement_axis_registry_gen>"
            "-DCOMMITTED=${_w2b_measurement_registry}"
            "-DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/registry_roundtrip_measurement"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/registry_roundtrip.cmake")
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS comdare_measurement_axis_registry_gen)
    set_tests_properties(test_measurement_axis_registry_roundtrip
        PROPERTIES LABELS "registry;roundtrip;measurement" TIMEOUT 120)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# GN-5 / L2 stale-skip-Guard (Roadmap 2026-07-19; #50-Stale-Green-Wurzel): reflect_versions-
# Instanziierung ueber ALLE 17 Kompositions-Achsen als Default-ctest. build_axis_variant_version_table()
# war bisher NUR von der CEB-Runtime instanziiert — eine Organ-Variante OHNE algo_version blieb im
# Default-ctest unsichtbar (stale-skip; skaliert mit golden-N). Schon der BUILD dieser TU ist der Guard
# (ill-formed W::algo_version-Zugriff bricht mit Typ-Namen); die Laufzeit-Checks belegen die 17-Achsen-
# Vollstaendigkeit gegen kCompositionAxisNames. Schwere TU (registry_to_axis_levels -> 23 topic_config_sets),
# isoliert retriggerbar — gleicher Include-/Define-Satz wie test_striktheit_axes_guard (das Blockvorbild).
# KEIN contract-/pmc-Label: der Test MUSS im Default-Sweep test:unit (-LE contract|pmc) laufen.
# ─────────────────────────────────────────────────────────────────────────────
comdare_add_test(test_reflect_versions_all17
    SOURCES test_reflect_versions_all17.cpp)
target_include_directories(test_reflect_versions_all17 PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_reflect_versions_all17 PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_reflect_versions_all17 PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_reflect_versions_all17 PROPERTIES LABELS "striktheit;algo_version;stale_skip" TIMEOUT 120)

# ---------------------------------------------------------------------------
# CX-W6 (Codex-Doppelreview 02.08.2026): die Flag-Grammatik-Wache ueber die VOLLE registrierte Organ-Population
# (auch deaktivierte Varianten), nicht nur die Enabled*-Emit-Liste. Schwere TU wie test_reflect_versions_all17
# (registry_to_axis_levels -> alle topic_config_sets); der BAU instanziiert guard_all_registered_organ_versions
# und IST die Wache. KEIN contract-/pmc-Label: MUSS im Default-Sweep test:unit laufen.
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# A1-VERSIONS-PIN der Allokator-Achse (2026-08-06, Review-Befund "Bump ohne Regressionsorakel";
# NACHTRAG: 2. Bump nach Lens-Pass/Owner-Entscheid -- 24 Strategien "v1.0.2c", 2 reallocate-lose
# Strategien [PmrResourceAllocator/VampirNfpAllocator] bleiben "v1.0.1c").
# Zwei Schichten: (A) 26x compile-harter Literal-Pin (je Strategie GENAU EINE der beiden Versionen,
# namentlich zugeordnet) + Registry-Seite (GENAU 26 Vendor, jeder auf der fuer ihn erwarteten Version)
# -- faengt den vergessenen, halben, zurueckgefallenen oder falsch zugeordneten Bump MIT dem Namen
# der Strategie. (B) Alt/Neu-Kontrast auf der Flaeche, an der der Rebuild-/Neu-Mess-Selektor
# entscheidet: compose_algo_signature (.algos-Sidecar, speist ueber kSubAxisValuesetSegment auch das
# Fingerprint-Preimage von dll_is_current) UND compose_organ_stamp_line. Bewegt sich die Signatur
# beim Zurueckdrehen (je Variante um GENAU EINEN Bump-Schritt) NICHT, waere der Bump wirkungslos.
# Nur-Lesen, TABU-neutral (der Kontrast arbeitet auf einer lokalen Kopie der Versions-Tabelle).
# Include-/Link-Satz wie der Geschwister-Test
# test_reflect_versions_all_registered (beide ziehen alle Kompositions-Registries).
# ---------------------------------------------------------------------------
comdare_add_test(test_a1_algo_version_pin_alloc_axis
    SOURCES test_a1_algo_version_pin_alloc_axis.cpp)
target_include_directories(test_a1_algo_version_pin_alloc_axis PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_a1_algo_version_pin_alloc_axis PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_a1_algo_version_pin_alloc_axis PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_a1_algo_version_pin_alloc_axis PROPERTIES
    LABELS "striktheit;algo_version;a1" TIMEOUT 120)

comdare_add_test(test_reflect_versions_all_registered
    SOURCES test_reflect_versions_all_registered.cpp)
target_include_directories(test_reflect_versions_all_registered PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_reflect_versions_all_registered PRIVATE Boost::mp11 comdare::workload_driver)
target_compile_definitions(test_reflect_versions_all_registered PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
set_tests_properties(test_reflect_versions_all_registered PROPERTIES
    LABELS "striktheit;algo_version;stale_skip" TIMEOUT 120)

# ---------------------------------------------------------------------------
# CX-W5 (Codex-Doppelreview 02.08.2026): der Planer-Selbst-Stempel (kPlannerVersion) faellt unter dieselbe
# Owner-Q10/Q3-Grammatik wie jede Achsen-Version. Leichte TU (profile_facade/planner/planner_version.hpp +
# measurement/algo_semver.hpp, header-only). Negativ-Probe: ce_owned_version_is_wellformed(kPlannerVersion) ist
# nach dem Fix ("v1.0.0") GREEN, vor dem Fix ("1.0.0") RED; die gerenderte Zeile bleibt byte-identisch.
# ---------------------------------------------------------------------------
comdare_add_test(test_planner_version_flag_grammatik
    SOURCES test_planner_version_flag_grammatik.cpp)
target_include_directories(test_planner_version_flag_grammatik PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_planner_version_flag_grammatik PRIVATE Boost::mp11)
set_tests_properties(test_planner_version_flag_grammatik PROPERTIES LABELS "striktheit;algo_version" TIMEOUT 60)

# ---------------------------------------------------------------------------
# G3 / #46b Lagerhaltung (Bestandslog-Kern, §62-B / §66 Lager-Gate). Header-only Module unter
# libs/cache_engine/builder/bestandslog/ + builder/artifact_transport/. Unit-Tests OHNE minio
# (Fake-Transport, deterministisch). B1 = Serialisierungs-Schicht (Dokument+XML-Emitter/Parser),
# haengt nur am common-DOM (xml_reader.hpp, <serialization/...>) -> keine Link-Deps, kein Boost.
# ---------------------------------------------------------------------------
comdare_add_test(test_g3_bestandslog_document
    SOURCES test_g3_bestandslog_document.cpp)
target_include_directories(test_g3_bestandslog_document PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")

# G3-B2 (S3): Lock + Transport-Naht + Record-Union-Merge (bestandslog_lock.hpp, header-only).
comdare_add_test(test_g3_bestandslog_lock
    SOURCES test_g3_bestandslog_lock.cpp)
target_include_directories(test_g3_bestandslog_lock PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")

# G3-B4 (S4): ETA-Rechner + Reservierungs-Lifecycle (eta_estimator.hpp, reservation_lifecycle.hpp).
comdare_add_test(test_g3_eta_estimator
    SOURCES test_g3_eta_estimator.cpp)
target_include_directories(test_g3_eta_estimator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
comdare_add_test(test_g3_reservation_lifecycle
    SOURCES test_g3_reservation_lifecycle.cpp)
target_include_directories(test_g3_reservation_lifecycle PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")

# A5 / F5-ETA-Paket: Belastbarkeits-Schwelle, Median-Projektion, Re-Kalibrierung, Kampagnen-Hochrechnung
# (eta_kalibrierung.hpp). Rein arithmetisch, zeit-/IO-frei -- der Wirk-Nachweis am echten Bau-Weg liegt in
# test_a5_eta_slice_verdrahtung.
comdare_add_test(test_a5_eta_kalibrierung
    SOURCES test_a5_eta_kalibrierung.cpp)
target_include_directories(test_a5_eta_kalibrierung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")

# G3-B5 (S5): async Batch-Planer + Slice-Queue (batch_planner.hpp, slice_queue.hpp). Nutzt std::thread.
comdare_add_test(test_g3_batch_planner
    SOURCES test_g3_batch_planner.cpp)
target_include_directories(test_g3_batch_planner PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_batch_planner PRIVATE Threads::Threads)

# G3-B6 (S6): RAM-Spool 256MB + portabler Writer-Thread (CT-Strategy SpoolWriter<Backend>).
# ram_spool.hpp/spool_writer.hpp/writer_backend_portable.hpp unter builder/artifact_transport/.
comdare_add_test(test_g3_ram_spool_writer
    SOURCES test_g3_ram_spool_writer.cpp)
target_include_directories(test_g3_ram_spool_writer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_ram_spool_writer PRIVATE Threads::Threads)

# G3-B7 (S7): io_uring-Writer-Backend gegen das VENDORED liburing (ext/io/liburing, Tag liburing-2.6).
# Das kleine statische Vendor-Target wird on-demand hereingezogen (NOT-TARGET-Guard in seiner
# CMakeLists) -- Reihenfolge garantiert, weil tests VOR ext/builder konfiguriert wird. Das Test-Target
# erzwingt die CT-Wahl io_uring via -DCOMDARE_WRITER_BACKEND_IO_URING -> io_uring baut im Gate; Test
# SKIPt bei ENOSYS.
if(NOT TARGET comdare_vendored_liburing)
    add_subdirectory("${PROJECT_SOURCE_DIR}/ext/io/liburing" "${CMAKE_BINARY_DIR}/_ext_io_liburing")
endif()
comdare_add_test(test_g3_writer_backend_io_uring
    SOURCES test_g3_writer_backend_io_uring.cpp)
target_include_directories(test_g3_writer_backend_io_uring PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_compile_definitions(test_g3_writer_backend_io_uring PRIVATE COMDARE_WRITER_BACKEND_IO_URING)
target_link_libraries(test_g3_writer_backend_io_uring PRIVATE Threads::Threads comdare::vendored_liburing)

# --- G2 Lane A Stempel-Tests (additiver Block; NUR Lane A erweitert diesen Block, nie fremde Bloecke) ---
# A7 (G2-3): .variant-Sidecar-Serializer (build_variant_sidecar.hpp) + dll_is_current-4.-Param-Skip-Matrix
# (build_orchestrator.hpp). Include-/Link-Satz wie die MCE24-Plain-Tests, da build_orchestrator experiment_tree
# und den XML-Config-Parser transitiv zieht; gtest zusaetzlich fuer die benannten Faelle.
# NACHGEFUEHRT 2026-08-05 (A2-Eichung, GATE 5): die "dll_is_current-4.-Param-Skip-Matrix" ist HISTORIK -- die
# 4-Parameter-Form existiert nicht mehr (F7 "NUR"), die Matrix-Abschnitte sind aus dieser TU entfallen. Sie
# prueft weiter den .variant-SERIALIZER und die A7-B-Mengen-Signatur; das Skip-Gate selbst beweist seither
# test_a2_sha512_skip_gate (Block direkt darunter).
add_executable(test_g2_variant_sidecar "test_g2_variant_sidecar.cpp")
target_compile_features(test_g2_variant_sidecar PRIVATE cxx_std_23)
target_include_directories(test_g2_variant_sidecar PRIVATE ${COMDARE_MCE24_INCLUDES})
target_link_libraries(test_g2_variant_sidecar PRIVATE gtest gtest_main Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_g2_variant_sidecar)
add_test(NAME test_g2_variant_sidecar COMMAND test_g2_variant_sidecar)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_g2_variant_sidecar)
set_tests_properties(test_g2_variant_sidecar PROPERTIES LABELS "g2;blut")

# A2 (GATE 5, F7 / Eich-Welle 2026-08-05): das SHA512-only-Skip-Gate. Beweist, dass dll_is_current genau EINEN
# Vergleich fuehrt (erwarteter CT-Fingerprint == `.fingerprint`-Sidecar), fail-closed ohne Anker, inkl. des
# Hydrations-Skips ueber pull_tier_binary und der Verdrahtung am echten Call-Site (provision_core).
# Include-/Link-Satz identisch zum G2-Block (build_orchestrator zieht experiment_tree + XML-Config-Parser
# transitiv; artifact_cache.hpp fuer den Hydrations-Beweis liegt unter derselben builder-Include-Wurzel).
add_executable(test_a2_sha512_skip_gate "test_a2_sha512_skip_gate.cpp")
target_compile_features(test_a2_sha512_skip_gate PRIVATE cxx_std_23)
target_include_directories(test_a2_sha512_skip_gate PRIVATE ${COMDARE_MCE24_INCLUDES})
target_link_libraries(test_a2_sha512_skip_gate PRIVATE gtest gtest_main Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_a2_sha512_skip_gate)
add_test(NAME test_a2_sha512_skip_gate COMMAND test_a2_sha512_skip_gate)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_a2_sha512_skip_gate)
set_tests_properties(test_a2_sha512_skip_gate PROPERTIES LABELS "a2;blut")

# Task #59 (Additiv-Vertrag GLIED [6]): der RICHTUNGS-BEHAFTETE bvset-Vergleich. Eigene TU statt eines
# Anbaus an die A2-Suite, weil sie einen ANDEREN Gegenstand hat: A2 beweist das Skip-GATE, diese hier den
# MENGEN-VERGLEICH, auf dem es seit #59 aufsetzt. Include-/Link-Satz identisch zum A2-Block -- der Pin auf
# die echte kDriverBuildVariantSignature zieht die drei CMake-generierten Achsen-Registries nach
# (driver_build_variant_signature.hpp), genau wie test_g2_variant_sidecar.
add_executable(test_bvset_teilmenge "test_bvset_teilmenge.cpp")
target_compile_features(test_bvset_teilmenge PRIVATE cxx_std_23)
target_include_directories(test_bvset_teilmenge PRIVATE ${COMDARE_MCE24_INCLUDES})
target_link_libraries(test_bvset_teilmenge PRIVATE gtest gtest_main Boost::mp11 comdare_builder_xml_config_parser)
comdare_set_platform_defines(test_bvset_teilmenge)
add_test(NAME test_bvset_teilmenge COMMAND test_bvset_teilmenge)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_bvset_teilmenge)
set_tests_properties(test_bvset_teilmenge PROPERTIES LABELS "a2;blut")

# G3-B3 (S2): Factory + SHA512-Index (bestandslog_factory.hpp, bestandslog_index.hpp). Braucht
# zusaetzlich libs/cache_engine/src fuer <sha512/ctsha512.hpp> (die EINE SHA512-Wahrheit gegen A1).
comdare_add_test(test_g3_sha512_index
    SOURCES test_g3_sha512_index.cpp)
target_include_directories(test_g3_sha512_index PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    # A13-M3/C3: bestandslog_index.hpp zieht abi/anatomy_fingerprint.hpp (EIN Preimage-Separator, keine
    # zweite Konkatenations-Regel) -> die abi-Include-Wurzel gehoert ab jetzt zu diesem Ziel.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")

# G3-I1 (S9a): Bestandslog-Registrierung/Dedup-Zustandsmaschine des Bau-Loops (builder_registration.hpp).
# Braucht libs/cache_engine/src (ctsha512 via Index) + Threads (Mutex im LagerRunState).
comdare_add_test(test_g3_builder_registration
    SOURCES test_g3_builder_registration.cpp)
target_include_directories(test_g3_builder_registration PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    # A13-M3/C3: bestandslog_index.hpp zieht abi/anatomy_fingerprint.hpp (EIN Preimage-Separator, keine
    # zweite Konkatenations-Regel) -> die abi-Include-Wurzel gehoert ab jetzt zu diesem Ziel.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_builder_registration PRIVATE Threads::Threads)

# Lager-TP1 (A) / G-E1 (ABNAHME-6): der Takeover-Konsument (builder_registration.hpp: epoch_from_
# utc_iso, collect_takeable_reservations, takeover_expired_reservations) mit FakeStore + Skript-Uhr.
# Include-/Link-Satz wie test_g3_builder_registration (ctsha512 via Index; Threads fuer den
# Retry-Jitter-Sleep im gelockten Zyklus).
comdare_add_test(test_g3_takeover_sweep
    SOURCES test_g3_takeover_sweep.cpp)
target_include_directories(test_g3_takeover_sweep PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    # A13-M3/C3: bestandslog_index.hpp zieht abi/anatomy_fingerprint.hpp (EIN Preimage-Separator, keine
    # zweite Konkatenations-Regel) -> die abi-Include-Wurzel gehoert ab jetzt zu diesem Ziel.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_takeover_sweep PRIVATE Threads::Threads)

# A1-Lager-Rest LB-0/LB-2/LB-3 (F9-Paketschnitt): die Pfad-Grammatik-Single-Source
# (bestandslog/lager_pfad_grammatik.hpp -- Ownership-Entscheid L5) und der Baum-Writer beider
# Realm-Wurzeln (bestandslog/lager_baum_writer.hpp). Der Writer zieht als EINZIGER die Layer-Kante zu
# experiment_tree/axis_path_serialization.hpp (18 Organ-Achsen) und zu abi/system_axis_order.hpp
# (bindende System-Ordnung, L22) -> Boost::mp11 gehoert deshalb zum Ziel, die Grammatik selbst bleibt
# dep-frei. ctsha512 kommt aus libs/cache_engine/src (deterministischer Kurzname bei Namens-Ueberlauf).
comdare_add_test(test_lb0_lager_pfad_grammatik
    SOURCES test_lb0_lager_pfad_grammatik.cpp)
target_include_directories(test_lb0_lager_pfad_grammatik PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_lb0_lager_pfad_grammatik PRIVATE Boost::mp11)

# A1-Lager-Rest LB-1: complete-heuristik.log je Knoten + Alleinschreiber-Lock + die
# Truncate-Zustandsmaschine (bestandslog/knoten_heuristik_log.hpp). Eigener Test wegen der
# Owner-Auflage A-1 ("Truncate ist die Bug-Quelle" -> eigene Test-Klasse). Include-/Link-Satz wie
# test_lb0_lager_pfad_grammatik (der Knoten-Log zieht den Baum-Writer und damit die 18 Organ-Achsen).
# LB-6 (2026-08-06): zusaetzlich Threads::Threads -- Abschnitt (7) der TU faehrt ECHTE std::thread
# gegen make_filesystem_ablage() (Spin-Lock-Konkurrenz auf dem realen Dateisystem, Owner-Auflage
# LB-6). Ohne das Ziel-Link scheitert der Test erst beim Linken, nicht beim Uebersetzen.
comdare_add_test(test_lb1_knoten_heuristik_log
    SOURCES test_lb1_knoten_heuristik_log.cpp)
target_include_directories(test_lb1_knoten_heuristik_log PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_lb1_knoten_heuristik_log PRIVATE Boost::mp11 Threads::Threads)

# A1-Lager-Rest S4: G-E3 (produktiver Messwert-Genus-Schreiber), G-E6 (Versions-Tag +
# syntax_version 3->4) und G-E7 (Dual-ccache-Ziel-Strategy + mc-Alias-Preflight). Reine
# Bestandslog-Schicht: Include-Satz wie test_g3_builder_registration (ctsha512 via Index,
# Threads fuer den Retry-Jitter im gelockten Zyklus), KEIN Boost/experiment_tree.
comdare_add_test(test_ge3_messwert_genus_und_ziel
    SOURCES test_ge3_messwert_genus_und_ziel.cpp)
target_include_directories(test_ge3_messwert_genus_und_ziel PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_ge3_messwert_genus_und_ziel PRIVATE Threads::Threads)

# A1-Lager-Rest S5: der LAUF-START-Claim-Check (G-E1/2.4-(8)/ABNAHME-6) und N8-Variante-B
# (per-Owner-Sidecar-Registrierung, bestandslog/registrierungs_sidecar.hpp). Include-/Link-Satz
# wie test_g3_takeover_sweep (ctsha512 via Index; Threads fuer den Retry-Jitter-Sleep) --
# registrierungs_sidecar.hpp zieht knoten_heuristik_log.hpp (EINE Zustands-/Nachweis-Doktrin) und
# darueber den Baum-Writer, also Boost::mp11.
comdare_add_test(test_ge1_claim_check_und_sidecar
    SOURCES test_ge1_claim_check_und_sidecar.cpp)
target_include_directories(test_ge1_claim_check_und_sidecar PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_ge1_claim_check_und_sidecar PRIVATE Threads::Threads Boost::mp11)

# G3-I1b (S9b-Planer): Planer-getriebener Slice-Bau-Kern (planer_driven_build.hpp). Nutzt std::thread.
comdare_add_test(test_g3_planer_driven
    SOURCES test_g3_planer_driven.cpp)
target_include_directories(test_g3_planer_driven PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    # T2-A/F4-NB: slice_plan_stamp bindet die Indexfolge ueber einen sha512-Digest (slice_index_digest,
    # planer_driven_build.hpp) -- dieselbe Primitive, aus der der Fingerprint entsteht. Deshalb gehoert
    # die ctsha512-Wurzel ab jetzt zu diesem Ziel (Muster test_g3_lager_presence).
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_planer_driven PRIVATE Threads::Threads)

# G5 (P-B): verify_remote_then_prune-Kern (prune_verdict + prunable_artifacts) in artifact_cache.hpp.
# Braucht libs/cache_engine/include (abi/measurement-Header von artifact_cache) + Threads (<thread>).
comdare_add_test(test_g3_prune
    SOURCES test_g3_prune.cpp)
target_include_directories(test_g3_prune PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_prune PRIVATE Threads::Threads)

# --- G3 Folge-A ---
# T1-Binder: Objekt-per-Key-Verben des ArtifactCache + make_bestand_transport (inert, ohne minio).
# Include-Satz wie test_g3_prune (artifact_cache zieht abi/measurement aus libs/cache_engine/include),
# zusaetzlich libs/cache_engine/src fuer ctsha512 (via bestandslog_index) + Threads (LagerRunState-Mutex).
comdare_add_test(test_g3_artifact_cache_transport
    SOURCES test_g3_artifact_cache_transport.cpp)
target_include_directories(test_g3_artifact_cache_transport PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_artifact_cache_transport PRIVATE Threads::Threads)

# T3-Naht: lager_presence (Index-Key-Naht + PresenceFn). Braucht libs/cache_engine/src (ctsha512 via
# Index) + Threads (LagerRunState-Mutex und der SlicePlanner-Thread im Abschluss-Test).
comdare_add_test(test_g3_lager_presence
    SOURCES test_g3_lager_presence.cpp)
target_include_directories(test_g3_lager_presence PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    # A13-M3/C3: bestandslog_index.hpp zieht abi/anatomy_fingerprint.hpp (EIN Preimage-Separator, keine
    # zweite Konkatenations-Regel) -> die abi-Include-Wurzel gehoert ab jetzt zu diesem Ziel.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_g3_lager_presence PRIVATE Threads::Threads)

# Fehlerklassen-Framework A15/FK-1 (Task #29, Owner-Entscheid E6 + Volles GO vom 02.08. auf die
# Bauplan-Empfehlung Q4): Marker-Zeile 'nicht_gebaut' fuer bau-gescheiterte Permutationen +
# SourceUnavailable-Zeile fuer Lade-/Dock-Fehler. Der Guard prueft die Marker-Form FIXTURE-UNABHAENGIG
# gegen lazy_csv_header() (Spalten-NAMEN statt einer hartkodierten Feldzahl) -- Lehre "gruene Tests
# zementieren alte Ordnung": eine neue Mess-Spalte faellt automatisch in die Token-Pflicht.
# Include-/Define-Satz wie test_lazy_resume_binary (header-only, KEIN dlopen/AnatomyModuleLoader).
add_executable(test_fk1_nicht_gebaut_marker "test_fk1_nicht_gebaut_marker.cpp")
target_compile_features(test_fk1_nicht_gebaut_marker PRIVATE cxx_std_23)
target_include_directories(test_fk1_nicht_gebaut_marker PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_fk1_nicht_gebaut_marker PRIVATE Boost::mp11)
comdare_set_platform_defines(test_fk1_nicht_gebaut_marker)
target_compile_definitions(test_fk1_nicht_gebaut_marker PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_fk1_nicht_gebaut_marker COMMAND test_fk1_nicht_gebaut_marker)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_fk1_nicht_gebaut_marker)
set_tests_properties(test_fk1_nicht_gebaut_marker PROPERTIES LABELS "fehlerklassen;standalone;measured")

# A8-S3 (2026-08-04, Qualitaets-Katalog KLASSE C): Wache der HOST-seitigen CSV-Spalten-Ereignisse
# (pmc_branch_misses = honest-0, NICHT erhoben -- offener Posten M-3a; op_<art>_p999_ns = Tail-Perzentile aus den
# IST-Vektoren; die drei nicht erhobenen T6-Groessen ehrlich als "n/a" statt stiller 0). Tragende
# Aussage: Header- und Zeilen-Zellenzahl sind GLEICH -- ein halb nachgezogener END-Append verschoebe
# jede Auswertungs-Spalte. Reiner Host-Pfad, kein Tier.
add_executable(test_a8s3_csv_klasse_c "test_a8s3_csv_klasse_c.cpp")
target_compile_features(test_a8s3_csv_klasse_c PRIVATE cxx_std_23)
target_include_directories(test_a8s3_csv_klasse_c PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_a8s3_csv_klasse_c PRIVATE Boost::mp11)
comdare_set_platform_defines(test_a8s3_csv_klasse_c)
target_compile_definitions(test_a8s3_csv_klasse_c PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_a8s3_csv_klasse_c COMMAND test_a8s3_csv_klasse_c)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_a8s3_csv_klasse_c)
set_tests_properties(test_a8s3_csv_klasse_c PROPERTIES LABELS "a8;csv;standalone;measured")

# MT-L4 (09.08.2026): RF-2 nachregistriert (Kanon-Abschnitt 70.2). Er lag seit 26.07. unregistriert und
# prueft GENAU DENSELBEN Renderer wie seine beiden Nachbarn hier (format_csv_row/lazy_csv_header,
# reiner Host-Pfad, kein Tier) -- deshalb steht er hier und nicht bei den Fehlerklassen-Guards.
# Bau-Zuschnitt identisch zum Nachbarn darueber, inklusive der Mess-Defines: sie beschreiben die
# uebersetzende TU (der Iterator-Header zieht den Mess-Pfad mit), nicht die geprueften Zeilen.
# WAS DIE REGISTRIERUNG SOFORT EINGEBRACHT HAT -- der Erstlauf war ROT, an genau EINER Zusicherung:
# der Test stand auf "18 Latenz-Zellen (6 Op-Arten x 3)", der Renderer schreibt inzwischen eine
# vierte Zelle je Op-Art (op_<art>_p999_ns) und damit 24. Die Zusicherung war seit dem Tag jener
# Erweiterung falsch und ist 14 Tage niemandem aufgefallen, weil die Datei nie gebaut wurde.
# Die Stelle ist repariert und holt ihren Nenner jetzt aus dem D2-Pendant statt aus einem Literal.
add_executable(test_rf2_admission_marker_inert "test_rf2_admission_marker_inert.cpp")
target_compile_features(test_rf2_admission_marker_inert PRIVATE cxx_std_23)
target_include_directories(test_rf2_admission_marker_inert PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_rf2_admission_marker_inert PRIVATE Boost::mp11)
comdare_set_platform_defines(test_rf2_admission_marker_inert)
target_compile_definitions(test_rf2_admission_marker_inert PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_rf2_admission_marker_inert COMMAND test_rf2_admission_marker_inert)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_rf2_admission_marker_inert)
set_tests_properties(test_rf2_admission_marker_inert PROPERTIES
    LABELS "fehlerklassen;csv;standalone;measured" TIMEOUT 60)

# M-1/H-B (06.08.2026): die Observer-Zellen der CSV sind "n/a" statt 0, wenn die Tier-Binary ohne
# Observer-Gate gebaut ist. Gleicher Bau-Zuschnitt wie der Nachbar oben (reiner Host-Pfad ueber
# format_csv_row/lazy_csv_header, kein Tier). Die Mess-Defines sind noetig, weil der Iterator-Header
# den Mess-Pfad mitzieht -- sie beschreiben die uebersetzende TU, nicht die geprueften Zeilen.
add_executable(test_m1hb_observer_zellen_ehrlich "test_m1hb_observer_zellen_ehrlich.cpp")
target_compile_features(test_m1hb_observer_zellen_ehrlich PRIVATE cxx_std_23)
target_include_directories(test_m1hb_observer_zellen_ehrlich PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_m1hb_observer_zellen_ehrlich PRIVATE Boost::mp11)
comdare_set_platform_defines(test_m1hb_observer_zellen_ehrlich)
target_compile_definitions(test_m1hb_observer_zellen_ehrlich PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
add_test(NAME test_m1hb_observer_zellen_ehrlich COMMAND test_m1hb_observer_zellen_ehrlich)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_m1hb_observer_zellen_ehrlich)
set_tests_properties(test_m1hb_observer_zellen_ehrlich PROPERTIES LABELS "m1;csv;messachse;standalone")

# E-04-P1 (Slice-Kanal, Marker-Familie v2, Owner-KERN "live sehen ... exakt WELCHE ... noch offen"):
# Guard der Zeilen-Grammatik des Live-Fortschritts-Kanals. Tragende Aussage: lane=/zelle=/fenster=
# sind Pflichtfelder und der Aggregator-Schluessel ist das TUPEL (zelle, fenster) -- der Guard weist
# negativ nach, dass fenster= allein zwei Perms EINER Lane verschmelzen wuerde. Die Tee-Filter-
# Erwartung wird aus kSliceMarkerTraceMarken abgeleitet (Lehre "gruene Tests zementieren alte
# Ordnung"). slice_marker.hpp ist std-only -> minimaler Include-Satz, kein Achsen-/Katalog-Zug.
add_executable(test_e04_slice_marker "test_e04_slice_marker.cpp")
target_compile_features(test_e04_slice_marker PRIVATE cxx_std_23)
target_include_directories(test_e04_slice_marker PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common")
add_test(NAME test_e04_slice_marker COMMAND test_e04_slice_marker)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_e04_slice_marker)
set_tests_properties(test_e04_slice_marker PROPERTIES LABELS "e04;slice_kanal;standalone")

# P-CACHELINE-LITERAL (2026-08-04, Vor-Anker-Pflichtposten der generalisierten Schnitt-Regel):
# Wache dafuer, dass die Cache-Line-Groesse des LayoutAwareChunkedStore die der cacheline-Unterachse
# IST und nicht wieder auseinanderlaeuft. Tragende Aussage: dieselben Daten, nur die Unterachse
# permutiert -> Record-Stride, Chunk-Bytes, Chunk-Alignment, FF2-Knotenbreite UND die CLU-Linienzahl
# folgen ihr. Am Alt-Stand (hartkodierte 64) BEISST die TU compile-hart und zur Laufzeit -- Lehre
# "gruene Tests zementieren alte Ordnung". Include-Satz wie test_ff2_node_width_subaxis (derselbe
# Achsen-/Store-Zug), aber ohne XML-Parser: die TU liest kein Profil.
add_executable(test_p_cacheline_store_line_source "test_p_cacheline_store_line_source.cpp")
target_compile_features(test_p_cacheline_store_line_source PRIVATE cxx_std_23)
target_include_directories(test_p_cacheline_store_line_source PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_p_cacheline_store_line_source PRIVATE Boost::mp11)
comdare_set_platform_defines(test_p_cacheline_store_line_source)
add_test(NAME test_p_cacheline_store_line_source COMMAND test_p_cacheline_store_line_source)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_p_cacheline_store_line_source)
set_tests_properties(test_p_cacheline_store_line_source PROPERTIES
    LABELS "axes;cacheline;p_cacheline;standalone" TIMEOUT 60)

# B14-NB3 (2026-08-06) -- SCAN-SEITE der KF-6-Auflage (D) + WRAPPER-AUDIT.
# Schwester-Wache zu test_p_cacheline_store_line_source: dort folgt der STORE der cacheline-Unterachse,
# hier der MESS-SCAN. Tragende Aussage: dieselben Bytes, nur die Unterachse permutiert -> der Stride des
# gepaddeten AoS-Scans folgt ihr. Am Stand vor B14-NB3 (hartes `kCacheLine = 64` in
# CacheLineAlignedMemoryLayout::scan_field_sum) fielen alle vier Line-Groessen auf denselben Stride --
# die Achse waere in der Messung wirkungslos gewesen. Zusaetzlich der Wrapper-Audit: die Bedingung, die
# abi_adapter::detail::layout_scan_stride_bytes als Bau-Regel traegt, wird hier gegen alle aktivierten
# Strategien, ihre Registry-Huellen UND eine bewusst nicht-forwardende Negativ-Probe gefahren.
# Kein XML-Parser, kein Profil: reine Achsen-/Scan-Aussage (Include-Satz wie test_p_cacheline_*).
add_executable(test_b14_layout_scan_line_subaxis "test_b14_layout_scan_line_subaxis.cpp")
target_compile_features(test_b14_layout_scan_line_subaxis PRIVATE cxx_std_23)
target_include_directories(test_b14_layout_scan_line_subaxis PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_b14_layout_scan_line_subaxis PRIVATE Boost::mp11)
comdare_set_platform_defines(test_b14_layout_scan_line_subaxis)
add_test(NAME test_b14_layout_scan_line_subaxis COMMAND test_b14_layout_scan_line_subaxis)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_b14_layout_scan_line_subaxis)
set_tests_properties(test_b14_layout_scan_line_subaxis PROPERTIES
    LABELS "axes;cacheline;memory_layout;b14;standalone" TIMEOUT 60)

# B14-NB3 (2026-08-06) -- die DREI abi_adapter-Mess-Pfade bei Line-Groesse 32/64/128/256 (Codex C-ii) und
# die Leck-Wache fuer den Layout-Scan-Puffer auf dem Exception-Pfad (Codex C-iii).
# (C-ii): die Puffer-Geometrie (B14-NB2) und der Scan-Stride (B14-NB3) waren bis hierher nur am
#   Achsen-Default B64 je gelaufen -- also genau dort, wo sie nichts aendern. Die TU instanziiert und
#   FAEHRT run_workload / _segmented / _segmented_v2 mit einer line-permutierten Komposition.
# (C-iii): nach erfolgreicher Puffer-Allokation fuehrte jede spaetere Exception in `catch (...) return 0`
#   OHNE Freigabe (real erreichbar: PoolResourceAllocator wirft laut eigener Deklaration bad_alloc im
#   Churn-Segment). Ein Sonden-Allokator erzwingt genau dieses Fenster und prueft die BILANZ des Blocks:
#   ohne den RAII-Waechter fehlen 1 MiB (B64) bzw. 4 MiB (B256) je Fehlschlag.
# Braucht COMDARE_MEASUREMENT_ON (Pfad A ist sonst nicht kompiliert) -- Definitionssatz wie
# test_abi_adapter_g3_batch2.
comdare_add_test(test_b14_abi_adapter_line_subaxis_paths
    SOURCES test_b14_abi_adapter_line_subaxis_paths.cpp)
target_include_directories(test_b14_abi_adapter_line_subaxis_paths PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_b14_abi_adapter_line_subaxis_paths PRIVATE Boost::mp11)
target_compile_definitions(test_b14_abi_adapter_line_subaxis_paths PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)
# B14-NB4: LABELS nachgezogen. comdare_add_test (cmake/gtest_setup.cmake) setzt NUR TIMEOUT, keine Labels --
# die Schwester-Wache oben traegt "b14", diese Bau-Variante blieb dadurch ohne. Wer `ctest -L b14` fahrt
# (der Weg, auf dem das Paket lokal nachgeprueft wird), bekaeme sonst die HAELFTE der B14-Wachen und
# haette trotzdem den Eindruck, das Paket geprueft zu haben. Der TIMEOUT der Helper-Zeile bleibt stehen
# (set_tests_properties ist additiv je Property).
set_tests_properties(test_b14_abi_adapter_line_subaxis_paths PROPERTIES
    LABELS "anatomy;abi;memory_layout;cacheline;b14")

# --- A9-S1 (xlsx-Writer, Vendor-Scheibe) -----------------------------------------------------------
# Rauchtest gegen die VENDORIERTEN Snapshots ext/io/libxlsxwriter (Tag v1.2.4) und ext/io/zlib
# (Tag v1.3.2). Beide Vendor-Targets werden on-demand hereingezogen (NOT-TARGET-Guard in ihren
# CMakeLists) -- exakt wie der liburing-Block weiter oben. Kein bestehendes Target bekommt dadurch
# eine Abhaengigkeit; ohne diesen Test wuerde kein einziges zlib-/xlsx-Objekt uebersetzt.
#
# zlib wird zusaetzlich EXPLIZIT gelinkt: der Test bringt seinen eigenen ZIP-Leser mit und braucht
# darum <zlib.h> selbst. Ueber comdare_vendored_xlsxwriter kaeme zlib nur als LINK_ONLY-Abhaengigkeit
# an (dort ist es PRIVATE gelinkt, damit es nicht in Consumer leckt) -- ohne den Include-Pfad.
if(NOT TARGET comdare_vendored_xlsxwriter)
    add_subdirectory("${PROJECT_SOURCE_DIR}/ext/io/libxlsxwriter" "${CMAKE_BINARY_DIR}/_ext_io_libxlsxwriter")
endif()
comdare_add_test(test_a9_xlsx_vendor_rauchtest
    SOURCES test_a9_xlsx_vendor_rauchtest.cpp)
target_link_libraries(test_a9_xlsx_vendor_rauchtest PRIVATE
    comdare::vendored_xlsxwriter comdare::vendored_zlib)
set_tests_properties(test_a9_xlsx_vendor_rauchtest PROPERTIES
    LABELS "a9;xlsx;vendor;lager")

# --- A9-S3 (xlsx-Writer, Writer-Kern) ---------------------------------------------------------------
# WritebackMethod jetzt mit VIERTEM Wert Xlsx (writeback_method_registry.hpp) -- reiner Header-Test,
# keine Vendor-Abhaengigkeit.
comdare_add_test(test_a9s3_writeback_method_xlsx
    SOURCES test_a9s3_writeback_method_xlsx.cpp)
target_include_directories(test_a9s3_writeback_method_xlsx PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
set_tests_properties(test_a9s3_writeback_method_xlsx PROPERTIES
    LABELS "a9;xlsx;lager;writeback_method")

# Die Sheet-Namens-/Zeilenlimit-Wachen + der CSV-Fallback (builder/lager_ablage/ergebnis_mappe.hpp) sind
# selbst header-only (reines std::ofstream), ABER ErgebnisMappenFactory::oeffne() (inline im Header)
# referenziert detail::erzeuge_xlsx_ergebnis_mappe() in JEDEM Aufruf-Pfad -- auch wenn dieser Test nur
# ErgebnisFormat::csv waehlt, ist die xlsx-Backend-Funktion ein ODR-Use und muss zur Linkzeit aufgeloest
# werden. Jeder Konsument der Factory (auch ein CSV-only-Aufrufer) linkt deshalb comdare::lager_ablage --
# NUR wer die Factory ueberhaupt nicht benutzt, bleibt frei vom Vendor-Link (Design-Kommentar in
# ergebnis_mappe.hpp, detail::erzeuge_xlsx_ergebnis_mappe).
comdare_add_test(test_a9s3_ergebnis_mappe_csv
    SOURCES test_a9s3_ergebnis_mappe_csv.cpp)
target_include_directories(test_a9s3_ergebnis_mappe_csv PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")
target_link_libraries(test_a9s3_ergebnis_mappe_csv PRIVATE comdare::lager_ablage)
set_tests_properties(test_a9s3_ergebnis_mappe_csv PROPERTIES
    LABELS "a9;xlsx;lager;csv;wachen")

# Das xlsx-Backend (xlsx_ergebnis_writer.cpp, EIGENE TU) braucht das comdare_lager_ablage-Target (PRIVATE
# gegen den Vendor gelinkt -- <xlsxwriter.h> leckt nicht in diesen Test). zlib wird zusaetzlich EXPLIZIT
# gelinkt: der Test bringt seinen eigenen ZIP-Leser mit (Muster test_a9_xlsx_vendor_rauchtest.cpp).
if(NOT TARGET comdare_vendored_xlsxwriter)
    add_subdirectory("${PROJECT_SOURCE_DIR}/ext/io/libxlsxwriter" "${CMAKE_BINARY_DIR}/_ext_io_libxlsxwriter")
endif()
comdare_add_test(test_a9s3_xlsx_ergebnis_writer
    SOURCES test_a9s3_xlsx_ergebnis_writer.cpp)
target_include_directories(test_a9s3_xlsx_ergebnis_writer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/ext/io/zlib")
target_link_libraries(test_a9s3_xlsx_ergebnis_writer PRIVATE
    comdare::lager_ablage comdare::vendored_zlib)
set_tests_properties(test_a9s3_xlsx_ergebnis_writer PROPERTIES
    LABELS "a9;xlsx;lager;writer_kern")

# A9-S3 Fassung 3 (Owner-Entscheid 08.08. + Nachtrag): compare/Macro/Micro-Blattsorte, interne
# Hyperlinks, IN/OUT-Checkpoint-Zeilen. Braucht denselben Vendor-Link wie test_a9s3_xlsx_ergebnis_writer
# (eigener ZIP-Leser fuer den Hyperlink-Rueck-Lese-Beleg).
comdare_add_test(test_a9s3_mess_ebene_blattsorte
    SOURCES test_a9s3_mess_ebene_blattsorte.cpp)
target_include_directories(test_a9s3_mess_ebene_blattsorte PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/ext/io/zlib")
target_link_libraries(test_a9s3_mess_ebene_blattsorte PRIVATE
    comdare::lager_ablage comdare::vendored_zlib)
set_tests_properties(test_a9s3_mess_ebene_blattsorte PROPERTIES
    LABELS "a9;xlsx;lager;fassung3;mess_ebene")

# -----------------------------------------------------------------------------
# D2 (2026-08-09) -- SELBSTTEST DER ABDECKUNGS-WACHE (T-6).
#
# Prueft scripts/ci_test_coverage_guard.sh gegen praeparierte Bau-Baeume: ein
# uebersprungener Registrierungs-Block MUSS Exit 4 geben und namentlich in der
# Ausgabe stehen; ein gelaufener Block darf das NICHT (T-4-Gegeneingang). Die
# Kennungen wuerfelt der Test frisch aus /dev/urandom (K13).
#
# WARUM ALS GOOGLE TEST und nicht als weitere Shell-Probe: Owner-Entscheid
# 2026-08-09 ("Skripte sagen gar nichts"). Damit faehrt die Pruef-Logik in Debug
# UND Release ueber ctest mit, ohne eigenen CI-Job.
#
# UNBEDINGT registriert -- kein if() um diesen Block. Ein Selbsttest gegen den
# stillen Nenner-Schwund darf nicht selbst in einem bedingten Block sitzen; die
# Windows-Ausnahme traegt der Test intern als GTEST_SKIP.
comdare_add_test(test_d2_abdeckungs_wache_nenner
    SOURCES test_d2_abdeckungs_wache_nenner.cpp)
target_compile_definitions(test_d2_abdeckungs_wache_nenner PRIVATE
    "COMDARE_D2_GUARD_SH=\"${PROJECT_SOURCE_DIR}/scripts/ci_test_coverage_guard.sh\""
    "COMDARE_D2_CTEST_BIN=\"${CMAKE_CTEST_COMMAND}\"")
set_tests_properties(test_d2_abdeckungs_wache_nenner PROPERTIES
    LABELS "wachen;d2;nenner;selbsttest")

# MT-L4/W0a (2026-08-10): SELBSTTEST DER ZWEITEN GEGENSTANDSART der Test-Registrierungs-
# Wache. Anlass war ce-Pipeline 15517: die Wache meldete zu Recht ROT, weil das Format
# ihrer Allowlist eine ISA-Gattierung strukturell nicht ausdruecken konnte -- Feld 2 nannte
# immer eine DATEI, der Fall haengt aber an einer HOST-EIGENSCHAFT.
#
# UNBEDINGT registriert -- kein if() um diesen Block, und ausdruecklich NICHT hinter
# COMDARE_HOST_RUNS_*. Ein Selbsttest der ISA-Erloschen-Pruefung, der selbst nur auf
# AVX-512-Hosts liefe, waere genau auf der Host-Klasse blind, fuer die er gebaut ist.
# Er wuerfelt seine Baeume und faehrt beide Richtungen; die Windows-Ausnahme traegt er
# intern als GTEST_SKIP.
comdare_add_test(test_mt_l4_registrierungs_wache_isa
    SOURCES test_mt_l4_registrierungs_wache_isa.cpp)
target_compile_definitions(test_mt_l4_registrierungs_wache_isa PRIVATE
    "COMDARE_MTL4_WACHE_SH=\"${PROJECT_SOURCE_DIR}/scripts/ci_test_registrierungs_wache.sh\""
    "COMDARE_MTL4_ALLOWLIST=\"${PROJECT_SOURCE_DIR}/scripts/ci_test_registrierungs_allowlist.txt\""
    "COMDARE_MTL4_TESTS_CMAKE=\"${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt\"")
set_tests_properties(test_mt_l4_registrierungs_wache_isa PROPERTIES
    LABELS "wachen;mt_l4;registrierung;selbsttest")
# -----------------------------------------------------------------------------
# Posten #48 (2026-08-10) -- DIE KUMULATIVE ACHSE DER DIFF-HYGIENE-WACHE (T-6).
#
# Prueft scripts/ci_diff_ascii_width_guard.sh gegen Wegwerf-Repos, in denen der
# Koeder IM BEREICH liegt und NICHT IM PUSH: der push-lokale Lauf ist dann gruen,
# der kumulative (--bereich) rot -- auf DEMSELBEN Baum. Das ist die Lage vom
# 09.08.2026 (ce-Pipeline 15498 gruen / 15501 rot, beide auf aebc4f2c, 58
# Verstoesse), hier auf Knopfdruck herstellbar statt einmal beobachtet.
# Die Marken wuerfelt der Test frisch aus /dev/urandom (K13).
#
# WARUM ALS GOOGLE TEST und nicht als weitere Shell-Probe: Owner-Entscheid
# 2026-08-09 ("Skripte sagen gar nichts"). Damit faehrt der Biss-Beweis in Debug
# UND Release ueber ctest mit, ohne eigenen CI-Job; die Shell-Menge waechst nicht.
#
# UNBEDINGT registriert -- kein if() um diesen Block, aus demselben Grund wie beim
# D2-Selbsttest darueber. Die Windows-Ausnahme traegt der Test intern als
# GTEST_SKIP (die Wache ist ein POSIX-sh-Skript).
comdare_add_test(test_hyg_kumulativer_bereich
    SOURCES test_hyg_kumulativer_bereich.cpp)
target_compile_definitions(test_hyg_kumulativer_bereich PRIVATE
    "COMDARE_HYG_GUARD_SH=\"${PROJECT_SOURCE_DIR}/scripts/ci_diff_ascii_width_guard.sh\"")
set_tests_properties(test_hyg_kumulativer_bereich PROPERTIES
    LABELS "wachen;hygiene;kumulativ;bereich;selbsttest")

# A9-S4: der Konsument (tools/mess_report/). Die drei Header sind self-contained (relative Includes
# "../../tools/mess_report/..." loesen unabhaengig von den Include-Dirs auf); nur der Render-Test
# zieht ergebnis_mappe.hpp (lager_ablage) und braucht deshalb denselben Vendor-Link wie oben.
#
# K13-Bissprobe (.stale-Wache): "der Koeder muss erst gegen das Werkzeug beissen" -- Test 1 der
# Datei belegt am nackten String, dass eine Substring-Vereinfachung der Endungs-Pruefung die
# .stale-Datei faelschlich durchgewunken haette, BEVOR Test 2 dieselbe Datei gegen die reale
# scanne_realm_root()-Funktion prueft.
comdare_add_test(test_a9s4_realm_scan_stale
    SOURCES test_a9s4_realm_scan_stale.cpp)
set_tests_properties(test_a9s4_realm_scan_stale PROPERTIES
    LABELS "a9;mess_report;stale;wachen")

# D3-6 (2026-08-10) -- die SCHWESTERSTELLE des anhang:forward-Selektors, hier in ce: der
# --realm-root-Scan suchte NUR den Archiv-Suffix ".result.csv" und konnte die Produktions-Ablage
# (bin_dir/"result.csv", cache_engine_builder_iterator.hpp:2950) rechnerisch nie treffen. Der Test
# holt seinen Nenner aus dem ERZEUGER, nicht aus dem Pruefling -- deshalb steht er neben, nicht in
# test_a9s4_realm_scan_stale.cpp (dessen Faelle fuettern ausschliesslich die Archivform).
comdare_add_test(test_d3_6_realm_selektor_produktionsablage
    SOURCES test_d3_6_realm_selektor_produktionsablage.cpp)
set_tests_properties(test_d3_6_realm_selektor_produktionsablage PROPERTIES
    LABELS "d3;mess_report;selektor;nenner;wachen")

comdare_add_test(test_a9s4_dynamic_axis_filter
    SOURCES test_a9s4_dynamic_axis_filter.cpp)
set_tests_properties(test_a9s4_dynamic_axis_filter PROPERTIES
    LABELS "a9;mess_report;cor;dynamik")

comdare_add_test(test_a9s4_mess_report_render
    SOURCES test_a9s4_mess_report_render.cpp)
target_include_directories(test_a9s4_mess_report_render PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")
target_link_libraries(test_a9s4_mess_report_render PRIVATE comdare::lager_ablage)
set_tests_properties(test_a9s4_mess_report_render PROPERTIES
    LABELS "a9;mess_report;render;plan;led61")

# Vierter Owner-Nachtrag Punkt 2 (08.08.): SKIP bei validem Bestand fuer exakt dieselbe Binary.
comdare_add_test(test_a9s4_skip_manifest
    SOURCES test_a9s4_skip_manifest.cpp)
target_include_directories(test_a9s4_skip_manifest PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")
target_link_libraries(test_a9s4_skip_manifest PRIVATE comdare::lager_ablage)
set_tests_properties(test_a9s4_skip_manifest PROPERTIES
    LABELS "a9;mess_report;skip;manifest")
# A9-S5 (2026-08-09) -- die NAHT: Format-Entscheidung + Feld-Adapter + die Mappe neben der offiziellen
# CSV (profile_facade/ergebnis_mappe_naht.hpp). Bis hierher hatte der xlsx-Writer NULL Produktions-
# Aufrufer; dieser Test ist der erste, der die beiden geschuldeten Zusicherungen prueft (leeres
# <writeback_methods> => echte xlsx / csv gewaehlt => flache Sheets, keine xlsx).
# Braucht comdare::lager_ablage wie jeder Konsument der Factory (ODR-Use von
# detail::erzeuge_xlsx_ergebnis_mappe auch im csv-Zweig -- s. Kommentar bei test_a9s3_ergebnis_mappe_csv).
comdare_add_test(test_a9s5_ergebnis_mappe_naht
    SOURCES test_a9s5_ergebnis_mappe_naht.cpp)
# W0b-Nachbesserung 2026-08-10: Abschnitt E liest die erzeugte .xlsx als ZIP zurueck (dimension ref
# und <row>-Zahl aus xl/worksheets/sheet1.xml) -- das Orakel am INHALT statt am Zaehler. Dafuer
# braucht dieses Ziel denselben Vendor-Link wie test_a9s3_xlsx_ergebnis_writer: zlib zum Inflaten.
target_include_directories(test_a9s5_ergebnis_mappe_naht PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${PROJECT_SOURCE_DIR}/ext/io/zlib")
target_link_libraries(test_a9s5_ergebnis_mappe_naht PRIVATE
    comdare::lager_ablage comdare::vendored_zlib)
set_tests_properties(test_a9s5_ergebnis_mappe_naht PROPERTIES
    LABELS "a9;xlsx;lager;naht;format_wahl")

# ===================================================================================================
# MS-1 / MS-2 (2026-08-09) -- MeasureStorage: die ZWEI ARENEN und ihr Allokations-Beweis.
#
# SELBSTCHECK: beide Ziele sind STANDALONE int main() ohne gtest. Das ist keine Bequemlichkeit,
# sondern Pflicht: MS-1 ersetzt global operator new/delete und zaehlt JEDE Allokation im Fenster --
# ein Test-Framework, das im selben Fenster einen String oder einen Vektor anfasst, machte die Zahl
# wertlos. Rezept gespiegelt von test_t15_drift_gate_messschleife (oben): eigener add_executable,
# eigener add_test, kein comdare_add_test (das haengt gtest an, gtest_setup.cmake:83).
#
# COMDARE_MS_HOT_QUELLE reicht die heissen Kopfdateien fuer die Text-Wache durch (Muster:
# COMDARE_T15_ITERATOR_QUELLE oben) -- die Wache prueft, dass im heissen Block weder new noch malloc
# noch std::vector steht. Sie schliesst die Luecke, die eine operator-new-Ersetzung strukturell
# offenlaesst (C-malloc ist fuer sie unsichtbar).
set(_ms_dir "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/measure_storage")

add_executable(test_ms1_arenen_kein_alloc_im_fenster "test_ms1_arenen_kein_alloc_im_fenster.cpp")
target_compile_features(test_ms1_arenen_kein_alloc_im_fenster PRIVATE cxx_std_23)
target_include_directories(test_ms1_arenen_kein_alloc_im_fenster PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
comdare_set_platform_defines(test_ms1_arenen_kein_alloc_im_fenster)
target_compile_definitions(test_ms1_arenen_kein_alloc_im_fenster PRIVATE
    "COMDARE_MS_QUELLE_MESS=\"${_ms_dir}/mess_arena.hpp\""
    "COMDARE_MS_QUELLE_STAPEL=\"${_ms_dir}/stapel_arena.hpp\"")
add_test(NAME test_ms1_arenen_kein_alloc_im_fenster COMMAND test_ms1_arenen_kein_alloc_im_fenster)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_ms1_arenen_kein_alloc_im_fenster)
set_tests_properties(test_ms1_arenen_kein_alloc_im_fenster PROPERTIES
    LABELS "ms;measure_storage;arena;kein_alloc;standalone")

add_executable(test_ms2_pre_touch_seitenfehler "test_ms2_pre_touch_seitenfehler.cpp")
target_compile_features(test_ms2_pre_touch_seitenfehler PRIVATE cxx_std_23)
target_include_directories(test_ms2_pre_touch_seitenfehler PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")
comdare_set_platform_defines(test_ms2_pre_touch_seitenfehler)
add_test(NAME test_ms2_pre_touch_seitenfehler COMMAND test_ms2_pre_touch_seitenfehler)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_ms2_pre_touch_seitenfehler)
set_tests_properties(test_ms2_pre_touch_seitenfehler PROPERTIES
    LABELS "ms;measure_storage;arena;seitenfehler;standalone")

unset(_ms_dir)

# ===================================================================================================
# CK-1 (2026-08-09) -- DIE ABNAHME DER MESSKETTE: checkpoint_measure + Durchreichung + Mess-Visitor.
#
# Owner-KERN 09.08.2026: "checkpoint measure ist das ZENTRALE MESSINSTRUMENT DES SYSTEMS, es MUSS
# GEBAUT WERDEN. Sonst gibt es keine Messungen."
#
# DIE ZWEI BELEG-OBJEKTE, und warum sie OBJECT-Bibliotheken sind und keine Tests:
# Die Zusage "bei deaktiviert entsteht KEIN Aufruf" wird AM OBJEKT belegt, nicht behauptet. Dafuer
# wird derselbe Quelltext (ck1_naht_koerper.hpp) ZWEIMAL uebersetzt -- einmal mit voller, einmal mit
# leerer Mess-Konfiguration -- und die Symboltabellen werden verglichen. Die beiden .cpp unterscheiden
# sich in GENAU EINER ZEILE; nur deshalb ist der Symbolvergleich eine Aussage ueber die KONFIGURATION
# und nicht ueber den Quelltext.
#
# Sie sind OBJECT-Bibliotheken, weil der Test die OBJEKTDATEIEN braucht (nm liest sie) und nicht ein
# gelinktes Programm: beim Linken verschwaende der Linker genau die Symbole, um die es geht.
add_library(ck1_naht_voll OBJECT "ck1_naht_voll.cpp")
target_compile_features(ck1_naht_voll PRIVATE cxx_std_23)
target_include_directories(ck1_naht_voll PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${CMAKE_CURRENT_SOURCE_DIR}")
comdare_set_platform_defines(ck1_naht_voll)

add_library(ck1_naht_leer OBJECT "ck1_naht_leer.cpp")
target_compile_features(ck1_naht_leer PRIVATE cxx_std_23)
target_include_directories(ck1_naht_leer PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${CMAKE_CURRENT_SOURCE_DIR}")
comdare_set_platform_defines(ck1_naht_leer)

# Der Test selbst: gtest, aber EIGENES main() -- deshalb `gtest` OHNE `gtest_main`. Das main() nimmt
# die beiden Objektpfade als Argumente entgegen; sie kommen damit aus FREMDER QUELLE (CMake-
# Generatorausdruck) und nicht aus einer Vermutung des Tests ueber das Bauverzeichnis (T-3).
#
# Kein comdare_add_test: dessen add_test() reicht keine Argumente durch. Rezept fuer die
# Argument-Uebergabe gespiegelt von test_check_size_cli_deckel, Rezept fuer die gtest-Anbindung von
# comdare_add_test (cmake/gtest_setup.cmake).
add_executable(test_ck1_messkette_koeder "test_ck1_messkette_koeder.cpp")
target_compile_features(test_ck1_messkette_koeder PRIVATE cxx_std_23)
target_include_directories(test_ck1_messkette_koeder PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${CMAKE_CURRENT_SOURCE_DIR}")
comdare_set_platform_defines(test_ck1_messkette_koeder)
target_link_libraries(test_ck1_messkette_koeder PRIVATE gtest)
# Die Beleg-Objekte muessen VOR dem Test gebaut sein -- sonst laeuft nm auf eine fehlende Datei und
# der Test meldete "Werkzeug kaputt", wo in Wahrheit nur die Reihenfolge fehlte.
add_dependencies(test_ck1_messkette_koeder ck1_naht_voll ck1_naht_leer)
add_test(NAME test_ck1_messkette_koeder COMMAND test_ck1_messkette_koeder
    "$<TARGET_OBJECTS:ck1_naht_voll>"
    "$<TARGET_OBJECTS:ck1_naht_leer>")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_ck1_messkette_koeder)
set_tests_properties(test_ck1_messkette_koeder PROPERTIES
    LABELS "ck;checkpoint_measure;messkette;durchreichung;mess_visitor" TIMEOUT 60)

# ===================================================================================================
# check-size (2026-08-09) -- die MENGENRECHNUNG des Planers gegen handgerechnete Orakel.
#
# SELBSTCHECK: STANDALONE int main() ohne gtest und ohne Bibliotheks-Anhang. Der Prueflung
# (profile_facade/planner/planner_mengen_types.hpp) ist ein reiner Header: er rechnet und berichtet,
# er liest kein XML und faehrt keinen Walk. Genau deshalb kann dieser Test seine Soll-Werte VON HAND
# setzen (T-3: die Grundgesamtheit stammt aus einer anderen Quelle als der Prueflung) -- er braucht
# weder Profil noch Registry noch DLL. Rezept gespiegelt von test_ms1/test_ms2 oben.
#
# Die beiden Include-Wurzeln: libs/cache_engine traegt sowohl profile_facade/planner/... als auch
# builder/measure_storage/checkpoint_speicher.hpp (die gerufene Bestands-Funktion).
add_executable(test_check_size_mengenrechnung "test_check_size_mengenrechnung.cpp")
target_compile_features(test_check_size_mengenrechnung PRIVATE cxx_std_23)
target_include_directories(test_check_size_mengenrechnung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")
comdare_set_platform_defines(test_check_size_mengenrechnung)
add_test(NAME test_check_size_mengenrechnung COMMAND test_check_size_mengenrechnung)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_check_size_mengenrechnung)
set_tests_properties(test_check_size_mengenrechnung PROPERTIES
    LABELS "check_size;planer;mengen;drift_gate;standalone")

# ===================================================================================================
# check-size CLI (09.08.2026, Lens-Befund B-1..B-3) -- die KOMMANDO-SCHICHT des Byte-Deckels.
#
# Der Rechner-Test oben deckt mengen_rechnen/mengen_bericht; ALLE vier Lens-Defekte lagen aber in der
# Schicht darueber (Argument-Schleife + run_check_size_guarded). Dieser Test faehrt darum die ECHTE
# Planer-Binary als Prozess: der Binary-Pfad reist als $<TARGET_FILE:...>-Argument (Muster
# test_adhoc_buildvariant_dll), das gueltige Profil als expliziter Quellbaum-Pfad -- NICHT aus der
# Umgebung geraten, damit der Test hermetisch gegen COMDARE_THESIS_PROFILE ist.
add_executable(test_check_size_cli_deckel "test_check_size_cli_deckel.cpp")
target_compile_features(test_check_size_cli_deckel PRIVATE cxx_std_23)
comdare_set_platform_defines(test_check_size_cli_deckel)
add_test(NAME test_check_size_cli_deckel COMMAND test_check_size_cli_deckel
    "$<TARGET_FILE:comdare_experiment_planner>"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/m3v2_study.profile.xml")
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_check_size_cli_deckel)
set_tests_properties(test_check_size_cli_deckel PROPERTIES
    LABELS "check_size;planer;cli;deckel;standalone" TIMEOUT 120)
# D5-5 HDR-VERDRAHTUNG (2026-08-09) -- die Thesis-Zusage "das HdrHistogram ERHEBT die Latenz-
# Perzentile" bekommt einen Produktions-Konsumenten in der AUSWERTUNG (nicht in der Erhebung).
# COMDARE_D55_MESSPFAD_ROOT ist ein QUELLPFAD, den der Test NUR LIEST (Muster wie
# COMDARE_D51_ZENSUS_ROOT_*): der Erhebungs-Pfad checkpoint_measure/measure_storage, in dem der
# HDR-Header 0-mal vorkommen muss. Link auf den Wrapper, weil der Test das ECHTE Histogramm fahrt.
comdare_add_test(test_d55_hdr_verdrahtung
    SOURCES test_d55_hdr_verdrahtung.cpp)
target_include_directories(test_d55_hdr_verdrahtung PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common")
target_link_libraries(test_d55_hdr_verdrahtung PRIVATE
    comdare_hdr_histogram_wrapper)
target_compile_definitions(test_d55_hdr_verdrahtung PRIVATE
    "COMDARE_D55_MESSPFAD_ROOT=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/builder/measure_storage\"")

# -- NAHT-1 BISS (09.08.2026, Owner-KERN "der Ansatz mit den SIDECARS IST FALSCH") ---------------------
# DIE MESS-NAHT AM GENUS-INTERFACE: der Host reicht einen Mess-Visitor HINEIN, das Tier berichtet in ihn.
# Zwei .so aus EINEM Quelltext (naht1_visitor_module.cpp) mit IDENTISCHER Komposition; der einzige
# Unterschied ist der Praeprozessor-Zustand der Mess-Gates. Die MESS-AN-Schwester muss beissen (echte
# Werte gegen das Host-Op-Buch), die MESS-AUS-Schwester darf NICHT (keine Flaeche, kein Symbol).
# JEDE Schwester in ihr EIGENES Verzeichnis: AnatomyModuleLoader::load_all laedt ein VERZEICHNIS, nicht
# eine Datei -- lagen beide zusammen, prueften die Faelle zufaellig dieselbe Binary.
# Praezedenz des Schwester-Bau-Verfahrens: r3_biss_modul_gates_an/-aus (R-3, 07.08.2026).
set(_naht1_an_dir  "${PROJECT_BINARY_DIR}/generated/naht1_modules_an")
set(_naht1_aus_dir "${PROJECT_BINARY_DIR}/generated/naht1_modules_aus")
foreach(_n1mod IN ITEMS naht1_modul_mess_an naht1_modul_mess_aus)
    add_library(${_n1mod} SHARED "naht1_visitor_module.cpp")
    target_compile_features(${_n1mod} PRIVATE cxx_std_23)
    target_include_directories(${_n1mod} PRIVATE
        "${PROJECT_SOURCE_DIR}/libs/cache_engine"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
        "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
        "${PROJECT_SOURCE_DIR}/libs/common"
        "${PROJECT_BINARY_DIR}/generated"
        ${COMDARE_ALL_AXIS_GENERATED_DIRS})
    target_link_libraries(${_n1mod} PRIVATE Boost::mp11)
    comdare_set_platform_defines(${_n1mod})
    set_target_properties(${_n1mod} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS ${_n1mod})
endforeach()
# DER DATEINAME IST TEIL DES VERTRAGS: AnatomyModuleLoader::load_all nimmt AUSSCHLIESSLICH Dateien
# mit dem Praefix "comdare_anatomy_perm_" (anatomy_module_loader.cpp:190, Muster aus
# anatomy_codegen.cmake). Ohne PREFIX "" + OUTPUT_NAME haetten die Schwestern "libnaht1_*.so"
# geheissen und der Loader haette sie STILL uebergangen -- load_all liefert dann status_ok mit NULL
# Handles, und der Koeder waere an einer leeren Menge "rot" geworden, ohne je die Naht zu beruehren.
# Das ist die Gegenprobe-Regel in Reinform: eine Null ist erst ein Befund, wenn belegt ist, dass das
# Verfahren ueberhaupt findet.
set_target_properties(naht1_modul_mess_an PROPERTIES
    PREFIX "" OUTPUT_NAME "comdare_anatomy_perm_naht1_an"
    LIBRARY_OUTPUT_DIRECTORY "${_naht1_an_dir}" RUNTIME_OUTPUT_DIRECTORY "${_naht1_an_dir}")
set_target_properties(naht1_modul_mess_aus PROPERTIES
    PREFIX "" OUTPUT_NAME "comdare_anatomy_perm_naht1_aus"
    LIBRARY_OUTPUT_DIRECTORY "${_naht1_aus_dir}" RUNTIME_OUTPUT_DIRECTORY "${_naht1_aus_dir}")
target_compile_definitions(naht1_modul_mess_an PRIVATE
    COMDARE_ANATOMY_MODULE_BUILD=1 COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
target_compile_definitions(naht1_modul_mess_aus PRIVATE
    COMDARE_ANATOMY_MODULE_BUILD=1 COMDARE_NAHT1_GATES_AUS=1)

comdare_add_test(test_naht1_mess_visitor_biss
    SOURCES test_naht1_mess_visitor_biss.cpp)
target_include_directories(test_naht1_mess_visitor_biss PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_naht1_mess_visitor_biss PRIVATE
    Boost::mp11
    comdare::anatomy_module_loader
    ${CMAKE_DL_LIBS})
add_dependencies(test_naht1_mess_visitor_biss naht1_modul_mess_an naht1_modul_mess_aus)
# DIE PFADE KOMMEN ALS COMPILE-DEFINITIONS, NICHT ALS CTEST-ARGUMENTE.
# BEFUND am Objekt (09.08.2026): `set_property(TEST <n> PROPERTY COMMAND ...)` wird von ctest
# IGNORIERT -- add_test() hat das Kommando bereits festgelegt, die Property landete zwar in
# CTestTestfile.cmake (mit korrekt aufgeloesten Generator-Ausdruecken), aber ctest fuhr weiter das
# argumentlose Kommando. Der Test lief daraufhin mit leeren Pfaden und wurde ROT -- ein Werkzeug-
# Defekt, der sich wie ein Mess-Befund anfuehlte. Praezedenz der tragfaehigen Form im Baum:
# COMDARE_R5G_ADHOC_DLL_DIR bei test_v41_anatomy_adhoc_dll_load.
target_compile_definitions(test_naht1_mess_visitor_biss PRIVATE
    "COMDARE_NAHT1_AN_SO=\"$<TARGET_FILE:naht1_modul_mess_an>\""
    "COMDARE_NAHT1_AUS_SO=\"$<TARGET_FILE:naht1_modul_mess_aus>\""
    "COMDARE_NAHT1_AN_DIR=\"${_naht1_an_dir}\""
    "COMDARE_NAHT1_AUS_DIR=\"${_naht1_aus_dir}\"")
set_tests_properties(test_naht1_mess_visitor_biss PROPERTIES
    TIMEOUT 300 LABELS "naht1;messnaht;genus_interface;visitor;dll_roundtrip;fehlerklassen")

# -----------------------------------------------------------------------------
# Warnungs-Review Runde 1c/2a (09.08.2026) -- die zwei Wachen zu den Funden, die DEFEKTE waren.
#   test_warn2a_block_id_bidirektional : block_id==axis auch auf dem PROFIL-Pfad (7 gemeldete
#       Stellen + 1, die kein Uebersetzer sieht, weil sie default-konstruiert statt aggregiert).
#   test_warn1_d2_fingerprint_koeder   : der nachgezogene, BEISSENDE Koeder zu D2 (die erste
#       Minimalprobe unterschied ihre zwei Hypothesen nicht).
# -----------------------------------------------------------------------------
comdare_add_test(test_warn2a_block_id_bidirektional
    SOURCES test_warn2a_block_id_bidirektional.cpp)
target_include_directories(test_warn2a_block_id_bidirektional PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization")
# Boost::mp11 EXPLIZIT: die TU zieht ueber experiment_tree/axis_path_serialization.hpp ein
# <boost/mp11.hpp>. Der vendorierte Pfad (cmake/third_party/boost_mp11/include) kommt AUSSCHLIESSLICH
# ueber dieses Target -- comdare_builder_xml_config_parser vererbt ihn nicht. Ohne die Zeile baut die
# TU nur dort, wo zusaetzlich ein SYSTEMWEITES boost liegt (prod1 hat eines, der CI-Container nicht):
# ce-Pipeline 15482, Job test:coverage-guard -> "error: 'boost' has not been declared", make inventar
# nach 4 Versuchen abgebrochen. Am Objekt erhoben: 88 Test-Targets ziehen den Header transitiv, 42
# linken Boost::mp11 nicht, 41 davon erben den Include-Pfad ueber eine Bibliothek -- dieses eine erbt
# gar nichts (flags.make: 0 boost-Includes). T-6 gefahren, Schwestermenge leer.
target_link_libraries(test_warn2a_block_id_bidirektional PRIVATE Boost::mp11 comdare_builder_xml_config_parser)
set_tests_properties(test_warn2a_block_id_bidirektional PROPERTIES
    LABELS "warnungen;experiment_tree;bidirektionalitaet" TIMEOUT 60)

comdare_add_test(test_warn1_d2_fingerprint_koeder
    SOURCES test_warn1_d2_fingerprint_koeder.cpp
    LIBRARIES comdare_experiment)
set_tests_properties(test_warn1_d2_fingerprint_koeder PROPERTIES
    LABELS "warnungen;speicher;koeder" TIMEOUT 60)

# -----------------------------------------------------------------------------
# D2-G1 (10.08.2026) -- DIE REGISTRIERUNG IST TEIL DES TESTS (T-7).
# Die Wache gegen die Klasse, die test_commands/test_engine_adapters unsichtbar gemacht hat:
# eine gtest-Binary, die gebaut wird, deren ctest-Eintrag aber fehlt oder ein NOT_BUILT-Platzhalter
# ist. Sie liest den QUELLBAUM als fremden Nenner (T-3) und die generierten CTestTestfile.cmake als
# Zaehler -- das Objekt auf der Platte, nicht eine Log-Zeile (V-8).
# Kein target_link_libraries noetig: die TU zieht nur <filesystem> und gtest.
# -----------------------------------------------------------------------------
comdare_add_test(test_d2g1_ctest_registrierung
    SOURCES test_d2g1_ctest_registrierung.cpp)
target_compile_definitions(test_d2g1_ctest_registrierung PRIVATE
    "COMDARE_D2G1_QUELLBAUM=\"${PROJECT_SOURCE_DIR}\""
    "COMDARE_D2G1_BAUBAUM=\"${PROJECT_BINARY_DIR}\"")
set_tests_properties(test_d2g1_ctest_registrierung PROPERTIES
    LABELS "d2g1;registrierung;ctest" TIMEOUT 120)
# T-6 (2026-08-10) -- DIE WACHEN-SELBSTTESTS IN DER HAUSFORM.
#
# BEFUND: eine frueher gemessene Zahl nannte "27 CI-gerufene Wachen ohne Selbsttest".
# Eine Wache, deren Biss niemand je geprueft hat, kann seit Monaten gruen melden, ohne
# je etwas gemessen zu haben -- genau diese Klasse hat am 09.08. viermal an einem Tag
# zugeschlagen.
#
# BAUFORM: Google Test, NICHT ein weiterer *.selbsttest.sh. Owner-KERN 09.08.2026:
# "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? ...
#  SKRIPTE SAGEN GAR NICHTS." Ein neuer Shell-Selbsttest vergroesserte genau die Menge,
# die weg soll. Als ctest-Ziel fahren diese Pruefungen in Debug UND Release mit, ohne
# einen eigenen CI-Job zu brauchen.
#
# ALLE VIER UNBEDINGT REGISTRIERT -- kein if() um diesen Block. Ein Selbsttest gegen
# stilles Erblinden darf nicht selbst in einem bedingten Block sitzen; die
# Windows-Ausnahme (die Wachen sind POSIX-sh) tragen die Tests intern als GTEST_SKIP.
#
# TIMEOUT 300 statt der 60 aus comdare_add_test: jeder Fall legt ein Wegwerf-Repo oder
# einen praeparierten Bau-Baum an und faehrt darauf 'ctest -N' bzw. 'git ls-files'.
# Das ist teurer als ein reiner In-Memory-Test und liegt auf keinem kritischen Pfad.
# -----------------------------------------------------------------------------

# (1) DIE ERHEBUNG. Der Nenner steht in der AUSGABE (V-1), nicht in einem Bericht.
comdare_add_test(test_t6_wachen_inventar
    SOURCES test_t6_wachen_inventar.cpp)
target_compile_definitions(test_t6_wachen_inventar PRIVATE
    "COMDARE_T6_QUELLBAUM=\"${PROJECT_SOURCE_DIR}\"")
set_tests_properties(test_t6_wachen_inventar PROPERTIES
    LABELS "wachen;t6;inventar;nenner" TIMEOUT 300)

# (2) P3 der Priorisierung -- die einzige Wache mit literalem Nenner.
comdare_add_test(test_t6_biss_achsen_roundtrip_wache
    SOURCES test_t6_biss_achsen_roundtrip_wache.cpp)
target_compile_definitions(test_t6_biss_achsen_roundtrip_wache PRIVATE
    "COMDARE_T6_ACHSEN_WACHE=\"${PROJECT_SOURCE_DIR}/scripts/ci_achsen_roundtrip_wache.sh\""
    "COMDARE_T6_CTEST_BIN=\"${CMAKE_CTEST_COMMAND}\"")
set_tests_properties(test_t6_biss_achsen_roundtrip_wache PROPERTIES
    LABELS "wachen;t6;selbsttest;achsen" TIMEOUT 300)

# (3) P1 der Priorisierung -- die Stufe, deren Ausfall den Abdeckungs-Nenner leert.
comdare_add_test(test_t6_biss_sichtbarkeits_wache
    SOURCES test_t6_biss_sichtbarkeits_wache.cpp)
target_compile_definitions(test_t6_biss_sichtbarkeits_wache PRIVATE
    "COMDARE_T6_SICHT_WACHE=\"${PROJECT_SOURCE_DIR}/scripts/ci_test_sichtbarkeit_wache.sh\""
    "COMDARE_T6_CTEST_BIN=\"${CMAKE_CTEST_COMMAND}\"")
set_tests_properties(test_t6_biss_sichtbarkeits_wache PROPERTIES
    LABELS "wachen;t6;selbsttest;sichtbarkeit" TIMEOUT 300)

# (4) P2 der Priorisierung -- die unterste Stufe: Quelldatei gegen Bauweg.
comdare_add_test(test_t6_biss_registrierungs_wache
    SOURCES test_t6_biss_registrierungs_wache.cpp)
target_compile_definitions(test_t6_biss_registrierungs_wache PRIVATE
    "COMDARE_T6_REG_WACHE=\"${PROJECT_SOURCE_DIR}/scripts/ci_test_registrierungs_wache.sh\"")
set_tests_properties(test_t6_biss_registrierungs_wache PROPERTIES
    LABELS "wachen;t6;selbsttest;registrierung" TIMEOUT 300)

# PA-1 (2026-08-10) -- die ZWEITE ERLOSCHEN-RICHTUNG derselben Wache.
# Der Nachbar oben deckt Richtung 1 ("der Gegenstand kehrt zurueck"). Diese TU deckt
# Richtung 2 ("der Gegenstand kann NIE zurueckkehren") -- die Klasse, an der vier
# Allowlist-Zeilen eine Freistellung mit unbegrenzter Laufzeit waren. Zwei Ziele und
# nicht eines, weil die Faelle verschiedene Gegenstaende brauchen: der Nachbar praepariert
# Waisen und Bauwege, dieser hier praepariert INDEX-Zustaende (Gitlink, .gitignore) und
# faehrt zusaetzlich die ECHTE committete Allowlist.
# COMDARE_PA1_REPO ist noetig, weil Fall (8) und (9) am echten Objekt messen.
comdare_add_test(test_pa1_tote_ausnahme
    SOURCES test_pa1_tote_ausnahme.cpp)
target_compile_definitions(test_pa1_tote_ausnahme PRIVATE
    "COMDARE_PA1_WACHE=\"${PROJECT_SOURCE_DIR}/scripts/ci_test_registrierungs_wache.sh\""
    "COMDARE_PA1_REPO=\"${PROJECT_SOURCE_DIR}\"")
set_tests_properties(test_pa1_tote_ausnahme PROPERTIES
    LABELS "wachen;t6;selbsttest;registrierung;pa1" TIMEOUT 300)
# ===================================================================================================
# W0b / ##25 DURCHSTICH (2026-08-10) -- die kleinste EHRLICHE Kette bis ins PDF, als Google Test.
#
# LOEST AB (nicht: loescht): die Shell-Deckung ci/durchstich_wache.sh + ci/durchstich_bissprobe.sh im
# super-Repo. Owner 09.08. woertlich: "Ich sehe einen Haufen shells statt vernuenftiger google tests
# ... Skripte sagen gar nichts." Diese TU deckt den Teil der Kette, den das ce BESITZT:
#   format_csv_row -> MappenNaht -> .xlsx + .csv auf Platte -> LaTeX-Tabelle -> pdflatex -> PDF.
#
# WARUM DER INCLUDE-/LINK-SATZ SO GROSS IST: die Kette hat vier Erzeuger, und der Test greift jeden
# von ihnen im ORIGINAL ab, statt ihn nachzubauen --
#   * cache_engine_builder_iterator.hpp  (lazy_csv_header / format_csv_row / LazyMeasuredRow),
#     deshalb der a8s3-Bauzuschnitt inkl. COMDARE_MEASUREMENT_ON=1 und der generierten Achsen-Dirs;
#   * ergebnis_mappe_naht.hpp + comdare::lager_ablage (die Format-Wahl und die Mappe);
#   * comdare_builder_xml_config_parser (der PRODUKTIONS-Parser fuer die getrackten Profile --
#     der Nenner von G1 kommt so aus einer FREMDEN Quelle, nicht aus dem Prueflung);
#   * comdare::latex_anhang_lib (das einzige tex-erzeugende Werkzeug des ce);
#   * comdare_vendored_zlib -- die .xlsx wird ENTPACKT und der Lauf-Token im Klartext gesucht.
#     Ohne zlib bliebe nur "die Datei faengt mit PK an", also Anwesenheit statt Aussage (T-2/V-8).
#
# COMDARE_DURCHSTICH_PROFIL_DIR: der Test soll den Profil-Ort nicht raten. Fehlt das Define, ist die
# Wache ROT (fail-closed) -- sie faellt nicht auf ein leises Gruen zurueck.
#
# COMDARE_DURCHSTICH_PDFLATEX: find_program zur CONFIGURE-Zeit. Ist pdflatex da, ist das PDF-Glied
# HART; ist es nicht da (ce-Bau-Image traegt kein TeX -- 0 Treffer auf latexmk|pdflatex|texlive in
# .gitlab-ci.yml, gemessen 10.08.2026), meldet der Test die Luecke MIT NENNER und ueberspringt genau
# dieses eine Glied. COMDARE_DURCHSTICH_PDF_PFLICHT=1 in der Umgebung macht daraus ein hartes Rot,
# ohne dass jemand die Quelle anfassen muss -- der Hebel liegt damit in der YAML (Lead), nicht hier.
# ===================================================================================================
# ZWEITES Profil-Inventar: wird das ce als Unterprojekt des super gebaut, liegt unter
# <super>/Code/experiment_config/thesis_profiles das F1-Durchstich-Profil -- genau die Datei, die den
# Mini-Lauf der F1-Lieferung konfiguriert. Sie ist NICHT Teil des ce, wird aber vom selben Owner-KERN
# regiert. Das Define entsteht NUR, wenn das Verzeichnis zur Configure-Zeit wirklich da ist (V-8: der
# Gegenstand entscheidet, kein Schalter). Im ce-Alleinbau bleibt es leer und der Nenner sagt das.
# Ein explizites -DCOMDARE_DURCHSTICH_PROFIL_DIR2=<pfad> hat Vorrang: so laesst sich die Wache auch
# ohne Unterprojekt-Bau auf ein fremdes Inventar richten (und genau so wurde sie belegt).
if(NOT DEFINED COMDARE_DURCHSTICH_PROFIL_DIR2)
    set(COMDARE_DURCHSTICH_PROFIL_DIR2 "")
    if(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
        if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/Code/experiment_config/thesis_profiles")
            set(COMDARE_DURCHSTICH_PROFIL_DIR2 "${CMAKE_SOURCE_DIR}/Code/experiment_config/thesis_profiles")
            message(STATUS "Durchstich: zweites Profil-Inventar gefunden: ${COMDARE_DURCHSTICH_PROFIL_DIR2}")
        endif()
    endif()
endif()

find_program(COMDARE_DURCHSTICH_PDFLATEX_EXE NAMES pdflatex)
if(NOT COMDARE_DURCHSTICH_PDFLATEX_EXE)
    set(COMDARE_DURCHSTICH_PDFLATEX_EXE "")
    message(STATUS "Durchstich: pdflatex NICHT gefunden (0 von 1) -- das PDF-Glied laeuft nur mit TeX.")
else()
    message(STATUS "Durchstich: pdflatex gefunden (1 von 1): ${COMDARE_DURCHSTICH_PDFLATEX_EXE}")
endif()

comdare_add_test(test_w0b_durchstich_kette
    SOURCES test_w0b_durchstich_kette.cpp)
target_include_directories(test_w0b_durchstich_kette PRIVATE
    ${COMDARE_MCE24_INCLUDES}
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${PROJECT_SOURCE_DIR}/tools/latex_anhang")
target_link_libraries(test_w0b_durchstich_kette PRIVATE
    Boost::mp11
    comdare::lager_ablage
    comdare_builder_xml_config_parser
    comdare::latex_anhang_lib
    comdare_vendored_zlib)
comdare_set_platform_defines(test_w0b_durchstich_kette)
target_compile_definitions(test_w0b_durchstich_kette PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1
    COMDARE_DURCHSTICH_PROFIL_DIR="${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles"
    COMDARE_DURCHSTICH_PROFIL_DIR2="${COMDARE_DURCHSTICH_PROFIL_DIR2}"
    COMDARE_DURCHSTICH_PDFLATEX="${COMDARE_DURCHSTICH_PDFLATEX_EXE}")
# TIMEOUT 300 statt der 60 aus comdare_add_test: ein pdflatex-Erstlauf zieht sein Format-File und
# braucht auf einem kalten Container mehr als eine Minute. Die 60 waeren ein Falsch-Rot.
set_tests_properties(test_w0b_durchstich_kette PROPERTIES
    LABELS "w0b;durchstich;kette;xlsx;pdf;koeder" TIMEOUT 300)

# ------------------------------------------------------------------------------------------------
# ANKER-WACHE (10.08.2026): Marke statt Ledger-Zeilennummer.
# Reiner Quellbaum-Scan -- KEINE Projekt-Bibliothek, KEIN Projekt-Header. Der Test haengt damit an
# nichts, was ihn stillegen koennte, und laeuft in JEDEM Baum (Debug wie Release, GCC wie clang).
# SELBSTCHECK: der Pfad steht ueber eine Zwischenvariable, nicht ausgeschrieben -- eine
# ausgeschriebene Definitions-Zeile risse die Diff-Hygiene-Wache (>120 Spalten), s. die Notiz zu
# _t15_tree weiter oben in dieser Datei.
comdare_add_test(test_anker_marke_statt_ledgerzeile
    SOURCES test_anker_marke_statt_ledgerzeile.cpp)
set(_anker_baum "${PROJECT_SOURCE_DIR}")
target_compile_definitions(test_anker_marke_statt_ledgerzeile PRIVATE
    "COMDARE_ANKER_QUELLBAUM=\"${_anker_baum}\"")
unset(_anker_baum)
set_tests_properties(test_anker_marke_statt_ledgerzeile PROPERTIES
    LABELS "anker;marke;ledger;hygiene;ratsche;koeder")
# P14 (2026-08-10): DER RIEGEL UNTER DER SELBSTWARTENDEN VERSIONS-TAXONOMIE.
#
# Die Klassen-Liste (a)..(j) im Kopf von measurement/algo_semver.hpp fuehrt alle ce-eigenen
# Versions-Quellen und erklaert sich selbst fuer "AUS DEM DIFF/GREP ABZULEITEN, NIE HANDGEPFLEGT"
# (algo_semver.hpp:119-120). Erzwungen war das bis heute NICHT -- gemessen an 95cb3039 fehlten DREI
# produktive Quellen in der Liste, obwohl sie beide Politik-Wachen tragen. Ab hier ist die Zusage ein
# Riegel: faellt eine Quelle aus der Liste oder kommt eine neue dazu, wird dieser Test rot und nennt
# die Datei.
#
# UNBEDINGT registriert -- kein if() um diesen Block. Eine Wache gegen stille Listen-Luecken darf
# nicht selbst in einem bedingten Block sitzen (dieselbe Lehre wie bei der D2-Abdeckungs-Wache).
#
# Der Test liest QUELLTEXT, nicht Bau-Artefakte: er braucht keine Bibliothek, nur die drei Pfade und
# den Header selbst als Prueflig (Kommentar-Defekte in der Liste brechen damit hier).
set(_tax_semver "${PROJECT_SOURCE_DIR}/libs/cache_engine/include/cache_engine/measurement")
comdare_add_test(test_vs_taxonomie_klassen_grep
    SOURCES test_vs_taxonomie_klassen_grep.cpp)
target_include_directories(test_vs_taxonomie_klassen_grep PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_definitions(test_vs_taxonomie_klassen_grep PRIVATE
    "COMDARE_TAX_QUELLE=\"${_tax_semver}/algo_semver.hpp\""
    "COMDARE_TAX_LIBS=\"${PROJECT_SOURCE_DIR}/libs\""
    "COMDARE_TAX_BAUM=\"${PROJECT_SOURCE_DIR}\"")
unset(_tax_semver)
set_tests_properties(test_vs_taxonomie_klassen_grep PROPERTIES
    LABELS "wachen;taxonomie;versionierung;selbstwartung;koeder")

# S-1 (P8): DER VERTRAGSTEST DER STEMPEL-BASISKLASSE (abi/stempel_basis.hpp) -- T-1..T-9.
#
# Beweist die 28 Matrixzellen gegen die fremde KON7-04-Quelle, die T-4-Gegeneingaenge (vertragslose/
# teilvertragliche Probe-Structs werden abgelehnt, organ@CEB-Riegel, stille Leere unmoeglich), die
# Baustein-Bindung (GENAU 10 Typen + PlanRegistryTrioAnnotation bleibt aussen), die byte-identische
# Delegation von planner_version_stamp()/ceb_version_stamp() auf die Erbin-Komposita (Byte-Orakel mit
# Praefix-Anker), den Dock-Nenner (5) und den g1-Ausgabe-Block (4 Zeilen non-empty).
# Include-/Link-Menge == test_m_w12_stamp_bausteine (dieselbe Stempel-Flaeche): die TU zieht
# builder/ceb_version_stamp.hpp und braucht deshalb die EINE Emissionsstelle
# comdare_measurement_combo_ct (Verdrahtungs-Wache, s. Kommentar am m_w12-Block).
add_executable(test_s1_stempel_basis_vertrag "test_s1_stempel_basis_vertrag.cpp")
target_compile_features(test_s1_stempel_basis_vertrag PRIVATE cxx_std_23)
target_include_directories(test_s1_stempel_basis_vertrag PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${COMDARE_ALL_AXIS_GENERATED_DIRS})
target_link_libraries(test_s1_stempel_basis_vertrag PRIVATE gtest gtest_main Boost::mp11
    comdare_measurement_combo_ct)
comdare_set_platform_defines(test_s1_stempel_basis_vertrag)
target_compile_definitions(test_s1_stempel_basis_vertrag PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
add_test(NAME test_s1_stempel_basis_vertrag COMMAND test_s1_stempel_basis_vertrag)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_s1_stempel_basis_vertrag)
set_tests_properties(test_s1_stempel_basis_vertrag PROPERTIES LABELS "s1;stempel;vertrag;koeder" TIMEOUT 60)

# =============================================================================
# S-3 / Paket P1 (Task #4, 2026-08-13): ORDNUNGS-RELATION + VORAUSSETZUNGS-WACHE + FREIGABE-NAHT.
# Strang-Marker 'S-3 / Paket P1' -- eigener Block am DATEIENDE (Kollisionsschutz gegen die Bloecke
# anderer Straenge; Namenskonvention test_s3_ordnung_*, s. Kopf der beiden TUs).
#
# (1) test_s3_ordnung_relation: CT-Batterie + Laufzeit-Spiegel fuer flag_menge_ist_teilmenge /
#     flag_menge_in_signatur (measurement/flag_menge_ordnung.hpp), voraussetzungen_erfuellt +
#     Einhaengung in ce_owned_version_is_wellformed (KON11-01, ungated) und die
#     Geschlossenheits-Wache signatur_ist_voraussetzungs_geschlossen. Header-only-Flaeche =>
#     nur der include-Baum, kein Link-Bedarf ueber gtest hinaus.
comdare_add_test(test_s3_ordnung_relation
    SOURCES test_s3_ordnung_relation.cpp)
target_include_directories(test_s3_ordnung_relation PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")
target_compile_definitions(test_s3_ordnung_relation PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1)
set_tests_properties(test_s3_ordnung_relation PROPERTIES
    LABELS "s3;ordnung;teilmenge;voraussetzung;wachen;koeder")

# (2) test_s3_ordnung_freigabe: die FREIGEBENDE SEITE (S-3c) -- belege_aktive_maschinen_deklaration
#     (profile_facade/maschinen_deklarations_naht.hpp) gegen die ECHTE golden-Maschinen-Menge
#     (experiment_golden_kern.xml, NUR GELESEN), C-3a-rsp-Spiegel, T-6 (9 Organ-Klassen x 2 Dialekte
#     x 2 Maschinen-Klassen + nicht deklarierter Hostname). Wiring-Muster = test_experiment_kern_seam
#     (dieselbe Parser-Flaeche).
comdare_add_test(test_s3_ordnung_freigabe
    SOURCES test_s3_ordnung_freigabe.cpp
    LIBRARIES comdare_builder_xml_config_parser)
target_include_directories(test_s3_ordnung_freigabe PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade")
target_compile_definitions(test_s3_ordnung_freigabe PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1
    "COMDARE_EXPERIMENT_GOLDEN_KERN=\"${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere/experiment_golden_kern.xml\"")
set_tests_properties(test_s3_ordnung_freigabe PROPERTIES
    LABELS "s3;ordnung;freigabe;maschinen;c3a;koeder")
