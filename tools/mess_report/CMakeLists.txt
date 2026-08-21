# tools/mess_report -- A9-S4: comdare-mess-report, der KONSUMENT des A9-S3-xlsx-Writers.
#
# A9-Doc Abschnitt 4.3 ("Haupt-Konsument = neues CLI tools/mess_report/") + Scheiben-Plan A9-S4
# (docs/architecture/20260803-a9_xlsx_writer_f3_soll_design.md:485). SCHLANK, weil die Substanz in
# mess_report_render.hpp/dynamic_axis_filter.hpp/realm_scan.hpp liegt (Haus-Muster wie
# apps/experiment_planner: die App ist nur der Host).
add_executable(comdare_mess_report main.cpp)
set_target_properties(comdare_mess_report PROPERTIES OUTPUT_NAME "comdare-mess-report")
target_compile_features(comdare_mess_report PRIVATE cxx_std_23)

# comdare::lager_ablage liefert transitiv (PUBLIC) die Include-Wurzeln fuer
# <builder/lager_ablage/ergebnis_mappe.hpp>, <builder/bestandslog/lager_pfad_grammatik.hpp> und
# <cache_engine/measurement/...> -- dieselben Wurzeln, die A9-S3 schon oeffentlich macht.
target_link_libraries(comdare_mess_report PRIVATE comdare::lager_ablage)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_mess_report)
endif()

# INSTALL-NAHT: dieselbe Vorlage + dieselbe Begruendung wie apps/experiment_planner/CMakeLists.txt
# (dort woertlich ausgefuehrt: super installiert per add_subdirectory(...EXCLUDE_FROM_ALL) + eigener
# install(PROGRAMS "$<TARGET_FILE:...>" ...)-Zeile im super-Baum; diese install(TARGETS)-Regel traegt
# NUR den ce-STANDALONE-Weg und ist im super-Sub-Build inert -- live an derselben Stelle gegengeprueft,
# dieselbe EXCLUDE_FROM_ALL-Direktorien-Ebene gilt hier). tools/mess_report ist eine Nutzer-CLI (der
# Konsument der Mess-Ergebnisse), kein internes Bau-Werkzeug -- Lead-Entscheid A9-S4-Plan Punkt 4:
# eigene install()-Regel wie der Planer, NICHT ueber den Makefile-PLANER_CLI-Pfad, separat in
# MANUAL_RUN.md dokumentiert (s. dort, neuer Abschnitt "comdare-mess-report").
include(GNUInstallDirs)
install(TARGETS comdare_mess_report RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle tools). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_mess_report)
