# W1 (F1-HART, Owner-KERN 05.08.2026): der Experiment-Planer als EIGENE Binary -- Traeger der
# Mess-Achsen-STUFE-1-RT-FREIGABE (Stufen-Doktrin, Ledger-Nachtraege mittag-9/-10/-11).
# BAUPLAN 20260719 Phase 1: comdare_experiment_planner, nach der catalog_codegen-Blaupause.
#
# Section 42 ("Eigentlich erhaelt die Cache Engine die XML und uebernimmt die gesamte Arbeit"): die App lebt in
# der ce-Welt. Section 31-A4: der CI-Cache kann kuenftig NUR dieses statische Target halten.
#
# SCHLANK, weil die Schwere in der Fassaden-Bibliothek liegt: comdare::profile_run_facade traegt die gesamte
# Planer-SUBSTANZ (Director, Stufe-1-Builder, RegistryTrio-Resolve, validate-Gate, planer_block-Lebenszyklus).
# Diese App ist NUR der Host (CLI-Dispatch, Profil-/Env-Aufloesung, Fassaden-Aufrufe). ZWEI MODULE, KEINE
# VERERBUNG: der CEB-Treiber (super comdare-messung-driver) haengt an DERSELBEN Bibliothek, nicht an dieser App.

add_executable(comdare_experiment_planner main.cpp)
set_target_properties(comdare_experiment_planner PROPERTIES
    OUTPUT_NAME "comdare-experiment-planner")
target_compile_features(comdare_experiment_planner PRIVATE cxx_std_23)

target_include_directories(comdare_experiment_planner PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"      # <cache_engine/measurement/...> (planner_version.hpp)
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"      # xml_config_parser/xml_reader.hpp (Root-Tag-Sniff)
    # W5: der status-Rueck-Leser zieht bestandslog_document.hpp, das <serialization/xml_config_parser/xml_reader.hpp>
    # inkludiert -- also die libs/common-WURZEL, nicht nur deren serialization/-Unterverzeichnis. Beide Wurzeln
    # stehen bewusst nebeneinander: die obere Zeile deckt die W1-Form ohne Praefix, diese die praefigierte Form.
    "${PROJECT_SOURCE_DIR}/libs/common")

# Die PUBLIC-Include-Wurzel des Fassaden-Targets (libs/cache_engine) liefert <profile_facade/...> und
# <builder/artifact_transport/artifact_cache.hpp>.
target_link_libraries(comdare_experiment_planner PRIVATE
    comdare::profile_run_facade)

# Default-Profil + validate-Registry-Pfade (Spiegel super Code/02_messung_driver/CMakeLists.txt:27-37).
# Die ce-Seite ist PROJECT_SOURCE_DIR-relativ -> im super-Sub-Build zeigen die Pfade auf dieselben Dateien.
# Der Makro-Name COMDARE_MESSUNG_DEFAULT_THESIS_PROFILE bleibt UNVERAENDERT, damit die umziehenden Code-Bloecke
# diff-minimal sind (Umbenennung = Nach-Abgabe-Hygiene-Kandidat).
target_compile_definitions(comdare_experiment_planner PRIVATE
    "COMDARE_MESSUNG_DEFAULT_THESIS_PROFILE=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/thesis_profiles/m3v2_study.profile.xml\""
    "COMDARE_CE_AXIS_REGISTRY_PATH=\"${PROJECT_SOURCE_DIR}/libs/cache_engine/algorithm_profiles/cache_engine_axis_registry.xml\"")

# 2-Registry-Kanon (Bruecke-I2): der prt-art-Registry-Pfad ist NICHT ce-Wissen (Baseline-Layering) -- er kommt
# im super-Sub-Build ueber die vererbte Directory-Scope-Variable COMDARE_PRT_ART_DIR herein. Im ce-STANDALONE
# ohne prt-art-Klon bleibt das Define ungesetzt und der #else-Zweig des validate liefert rc 5 -- exakt die
# Paritaet zum heutigen #ifdef-Muster des Treibers (deklariert, kein stiller Funktionsverlust).
if(DEFINED COMDARE_PRT_ART_DIR)
    target_compile_definitions(comdare_experiment_planner PRIVATE
        "COMDARE_PRT_AXIS_REGISTRY_PATH=\"${COMDARE_PRT_ART_DIR}/prt_art/algorithm_profiles/prt_art_axis_registry.xml\"")
endif()

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_experiment_planner)
endif()
