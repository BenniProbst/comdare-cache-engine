# V41.F.6.1.R6 — Prüf-Dock (builder-seitiger per-Gattung ABI-Mess-Übergang, Doku 24 §8.8)
#
# Header-only INTERFACE-Target: verkabelt die bereits existierenden anatomy- + anatomy_commands- +
# anatomy_module_loader-Bausteine hinter einen uniformen per-Gattung-Vertrag (IPruefDock). KEIN Neubau,
# nur Benennung/Organisation der vorhandenen ABI-Mess-Schnittstelle.

add_library(comdare_ce_pruef_dock INTERFACE)
add_library(comdare::ce_pruef_dock ALIAS comdare_ce_pruef_dock)

target_include_directories(comdare_ce_pruef_dock INTERFACE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src")   # measurement/measurable_concept.hpp (transitiv über observable_composed_search)

target_compile_features(comdare_ce_pruef_dock INTERFACE cxx_std_23)

# ALLE DREI KANTEN HART, KEIN if(TARGET) -- A2.5-Fix F-2 (ce_anatomy_drive) und Fix-Runde 2
# (Loader + mp11, Lens-Fund: zwei Waechter derselben Bauform standen noch).
#
# WARUM: if(TARGET) wird zur PARSE-Zeit ausgewertet. Bei ce_anatomy_drive war das ein TOTER
# ZWEIG -- libs/cache_engine/CMakeLists.txt bindet builder/ VOR anatomy_drive/ ein, das Target
# existierte hier also noch nicht, die Bedingung war false, und die Kante entstand nie, lautlos.
# Bei Loader und mp11 war dieselbe Bauform heute zufaellig wahr; "zufaellig wahr" ist kein
# Zustand, auf den sich eine PFLICHT-Kante stuetzen darf, und ein Wachposten, der eine Pflicht
# als Wahlmoeglichkeit schreibt, verschweigt beim naechsten Umbau genau den Bruch, den er
# melden sollte. target_link_libraries loest Target-Namen zur GENERATE-Zeit auf: reihenfolge-
# unabhaengig, und ein Tippfehler bricht laut statt still ("ERST LAUTE COMPILE-FEHLER").
#
# OPTIONAL IST KEINE DAVON, am Objekt geprueft: die Schwester-Datei anatomy_drive/CMakeLists.txt
# zieht Loader UND mp11 bereits hart, und der Vollbau laeuft mit ihr auf beiden Compilern durch --
# beide Targets sind zur Generate-Zeit also zuverlaessig da.
#
#   anatomy_module_loader : der Mess-Treiber konstruiert/zerstoert AnatomyModuleHandle,
#                           dessen unload() im Loader-.cpp liegt.
#   ce_anatomy_drive      : K2 (Owner 09.08.2026) -- Status-Codes + Antriebs-Buendel liegen
#                           stufen-neutral.
#   Boost::mp11           : Meta-Listen der Gattungs-Registry.
target_link_libraries(comdare_ce_pruef_dock INTERFACE
    comdare::anatomy_module_loader
    comdare::ce_anatomy_drive
    Boost::mp11)

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
