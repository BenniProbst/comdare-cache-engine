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
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"          # V-08R: <sha256/ctsha.hpp> (planner_version.hpp)
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

# F-B3 (Codex-Nachreview W1/W2, Ledger-Nachtrag 05.08.2026 nachmittag-7): INSTALL-NAHT.
# Der W1-Rollen-Schnitt hat die Planer-Kommandos (validate | plan dump/ci/cmake | cache-key | fingerprint) AUS
# dem installierten comdare-messung-driver ENTFERNT. Ohne eine eigene install()-Regel verliert jedes installierte
# Deployment diese Kommandos ersatzlos -- der Rollen-Schnitt waere dort ein Funktionsverlust statt einer Teilung.
#
# ORTS-BEGRUENDUNG (die Vorlage steht NICHT in apps/): im ce-Repo installiert HEUTE keine einzige Bestands-App
# (apps/*/CMakeLists.txt tragen kein install(); die einzigen install()-Regeln liegen in den beiden VENDORED
# Paketen libs/common/platform/vendor/comdare-{simd,platform} und folgen deren eigener Export-Konvention --
# kein Muster fuer eine Repo-App). Die passende Vorlage ist die TREIBER-INSTALL-NAHT im super, die dieselbe
# Mess-Werkzeugkette traegt: super Code/02_messung_driver/CMakeLists.txt
#   install(TARGETS messung_driver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# ueber include(GNUInstallDirs) in super Code/CMakeLists.txt. Diese Form wird hier 1:1 uebernommen -- die
# Planer-Binary landet damit im selben bin-Layout wie der Treiber, dessen Rolle sie sich teilt.
#
# include(GNUInstallDirs) steht bewusst HIER und nicht in der ce-Wurzel: im super-Sub-Build ist das Modul bereits
# geladen (Cache-Variablen gesetzt, Re-Include folgenlos), im ce-STANDALONE ist es die einzige Stelle, an der
# CMAKE_INSTALL_BINDIR ueberhaupt entsteht. Damit bleibt die Naht in BEIDEN Baumformen definiert und die ce-Wurzel
# unangetastet (kein neues repo-weites Install-Layout unter Frist -- das waere eine eigene Entscheidung).
#
# REICHWEITE, EHRLICH VERMESSEN (nicht raten -- am Objekt geprueft): diese Regel wirkt im ce-STANDALONE-Install
# (Beleg: `cmake --install <build>` legt bin/comdare-experiment-planner an). Im SUPER-SUB-BUILD ist sie INERT,
# denn super zieht die ce per add_subdirectory(... EXCLUDE_FROM_ALL) (super Code/CMakeLists.txt:196/213/222), und
# EXCLUDE_FROM_ALL nimmt einer Unterverzeichnis-Ebene NICHT NUR den Default-Bau, sondern AUCH ihre install()-Regeln
# aus dem Install-Ziel des Elternprojekts (live gegengeprueft: dieselbe Regel installiert ohne EXCLUDE_FROM_ALL,
# mit EXCLUDE_FROM_ALL installiert sie auch dann nicht, wenn das Target zuvor explizit gebaut wurde).
# FOLGE: der Codex-Befund B3 ist fuer den ce-Install-Weg GESCHLOSSEN, fuer das SUPER-Deployment NICHT -- dort
# braeuchte es eine super-seitige Naht (eigene install()-Regel neben der des Treibers, oder die Aufhebung von
# EXCLUDE_FROM_ALL fuer genau dieses Target). Das ist eine SUPER-Aenderung und liegt ausserhalb dieses
# READ-ONLY-Auftrags -- als offener Punkt gemeldet statt stillschweigend als erledigt gefuehrt.
#
# NACHTRAG (05.08.2026 abend, w2fix-Nachbesserung): der offene Punkt IST super-seitig geschlossen --
# super Code/CMakeLists.txt installiert die fertige Planer-Datei per
#   install(PROGRAMS "$<TARGET_FILE:comdare_experiment_planner>" DESTINATION "${CMAKE_INSTALL_BINDIR}")
# direkt hinter seinem add_subdirectory(... EXCLUDE_FROM_ALL) und holt das Target per Target-Property
# EXCLUDE_FROM_ALL=FALSE ins super-ALL zurueck (Bau-vor-Install-Naht). Die install(TARGETS)-Regel HIER
# bleibt im super-Sub-Build weiterhin INERT (die DIRECTORY-Ebene entscheidet ueber die Aufnahme der
# install()-Regeln, die Target-Property aendert daran nichts -- live gegengeprueft: genau EINE
# Install-Zeile im super-Install) und traegt allein den ce-STANDALONE-Weg.
include(GNUInstallDirs)
install(TARGETS comdare_experiment_planner RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
