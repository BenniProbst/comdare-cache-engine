# TRAEGER-STUFE 1: PLANNER (Skeleton DESIGN #29 Par. 4.2, super 85c1174d; SUBSTANZ-EINZUG S-8-Kopf
# T-NEU-6, 2026-08-20). Heutiger Bestand lebt als profile_facade/ im Monolithen (Besitzkarte Par. 2a);
# KEIN Datei-Umzug des Bestands vor W7/#88. NEUBAUTEN entstehen HIER unter der Include-Wurzel
# <traeger/planner/...> (Bruecken-Regel Par. 4.3, keine Alt-Wurzeln).
#
# S-8-Kopf-Substanz (B-01/B-14/B-17-Geruest, header-only):
#   include/traeger/planner/naht_nachrichten.hpp   B-01 Naht-Schablone (3 Klassen hinauf, KON50/51/52)
#   include/traeger/planner/steuerdock.hpp         B-14 sechs Steuerdocks (Typen + Registry)
#   include/traeger/planner/traeger_rakete.hpp     Template-Method-Geruest (KEINE YAML, KON16/25)
add_library(comdare_planner INTERFACE)
target_include_directories(comdare_planner INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(comdare_planner INTERFACE cxx_std_23)

# ---------------------------------------------------------------------------------------------
# R-01 PLANER-SPLIT (eigene Binary VOR Abgabe; Schnitt-Spez = DESIGN #29) -- SCHNITT VORBEREITET:
#   (1) comdare_planner traegt ab jetzt die Stufe-1-NEUBAUTEN unter eigener Include-Wurzel; die
#       Planer-CLI (apps/experiment_planner) LINKT dieses Ziel bereits -- neue Planer-Substanz
#       waechst damit AUF der Traeger-Seite an, nicht im Monolithen.
#   (2) VOLLZUG (deklariert, W7/#88 -- NICHT hier): profile_facade/-Substanz wandert in dieses
#       Unterprojekt (INTERFACE -> STATIC-Ziel mit Quellen), die CLI linkt dann comdare_planner
#       STATT comdare::profile_run_facade; die K1-Brueckenkanten (Par. 3c) loesen sich dort auf.
#       Bis dahin bleibt dieses Ziel INTERFACE/header-only -- bewusst KEINE zweite Wahrheit neben
#       der Fassaden-Bibliothek.
# ---------------------------------------------------------------------------------------------
