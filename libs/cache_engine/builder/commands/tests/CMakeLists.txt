# V32.JJ.1+ (2026-05-21) - Tests fuer cache_engine_commands_v32
# Bridge zur globalen Test-Discovery via tests/CMakeLists.txt.
#
# Targets:
#   test_commands              (V32.II.2)
#   test_engine_adapters       (V34.A.1)

# D2-G1 (10.08.2026): include(GoogleTest) ENTFERNT. Das Modul liefert ausschliesslich
# gtest_discover_tests/gtest_add_tests -- beides benutzt diese Datei seit der Umstellung auf das
# add_test-Muster nicht mehr, und im ganzen Repo benutzt es sonst niemand (gemessen 10.08.2026:
# 0 Fundstellen fuer gtest_add_tests, 0 weitere include(GoogleTest)). Die Zeile stehen zu lassen
# waere eine geladene Waffe: sie macht gtest_discover_tests hier wieder aufrufbar, ohne dass ein
# Leser den verworfenen Weg (cmake/gtest_setup.cmake:56-82) ueberhaupt noch sieht.

# V32-Bit-Bank GETILGT (User-GO 2026-07-13): test_permutation_flags_v32 +
# permutation_flags_v32.hpp + abi_v1_to_v2_mapper.hpp entfernt — die Bit-Bank war
# ein nicht-standalone Import/Export-Feature des B+-Baums; E4-XML ist der
# Goldstandard (ersetzt nichts). cache_engine_commands_v32 bleibt (breitere
# Command-Pattern-Lib, unabhaengig von der Bit-Bank).

# Command-Pattern Smoke + AxisLibraryRegistry + AutoPermutator (6 Tests)
add_executable(test_commands test_commands.cpp)
target_link_libraries(test_commands PRIVATE
    cache_engine_commands_v32
    GTest::gtest_main
)
target_compile_features(test_commands PRIVATE cxx_std_23)

# V34.A.1 (2026-05-21) - Engine-Adapter Tests
add_executable(test_engine_adapters test_engine_adapters.cpp)
target_link_libraries(test_engine_adapters PRIVATE
    cache_engine_commands_v32
    GTest::gtest_main
)
target_include_directories(test_engine_adapters PRIVATE
    ${PROJECT_SOURCE_DIR}/libs/cache_engine/include
)
target_compile_features(test_engine_adapters PRIVATE cxx_std_23)

# D2-G1 (10.08.2026): EIN add_test JE gtest-BINARY -- das im Repo dokumentierte Muster.
#
# WAS HIER BIS HEUTE STAND: ein if(COMMAND gtest_discover_tests)-Zweig. Da GoogleTest oben per
# include(GoogleTest) geladen wird, war die Bedingung IMMER wahr -- der else()-Zweig mit den beiden
# add_test war seit dem 21.05.2026 toter Code, der nie ausgefuehrt wurde.
#
# WARUM DAS WEG MUSS, und zwar nicht als Geschmacksfrage: cmake/gtest_setup.cmake:56-82 hat
# gtest_discover_tests fuer dieses Projekt AUSDRUECKLICH VERWORFEN und begruendet (POST_BUILD fuehrt
# jede Test-Binary schon zur Bauzeit aus; PRE_TEST erzeugt eine Defer-Datei, die ein blankes
# `ctest -N` mit Exit 8 und 0 enumerierten Tests abbrechen laesst). Diese Datei war die EINZIGE
# Stelle im ganzen Repo, die es dennoch weiterbenutzte -- gemessen am 10.08.2026: 2 echte Aufrufe,
# alle uebrigen 4 Fundstellen im Baum sind Kommentare.
#
# DER MESSBARE SCHADEN, im frisch konfigurierten, NICHT gebauten Baum literal abgelesen:
#     Test   #1: test_commands_NOT_BUILT
#     Test   #2: test_engine_adapters_NOT_BUILT
# Zwei Eintraege, die beim Lauf mit "Could not find executable" FEHLSCHLAGEN, waehrend die 30 echten
# gtest-Faelle in ctest fehlten. Die Discovery ersetzt sie erst NACH einem vollen Bau durch 30
# Einzeleintraege -- die Sichtbarkeit hing also am gewaehlten Bauweg statt am Bauplan. Ein Job, der
# comdare_tests statt 'all' baut, stellt den Daueralarm wieder her.
#
# WAS SICH DAMIT AENDERT, ausdruecklich benannt: die ctest-GRANULARITAET wird pro Binary statt pro
# gtest-Fall -- `ctest -N` zeigt hier 2 Eintraege statt 30. Die 30 Faelle LAUFEN unveraendert; gtest_main
# fuehrt beim Start der Binary alle aus, gleiche Assertions, gleiche Exit-Bedeutung. Genau so haelt es
# der gesamte Rest des Repos (~330 Eintraege ueber comdare_add_test). Eine Aufschluesselung je Fall
# bleibt ueber --gtest_filter auf der Binary moeglich.
#
# TIMEOUT 60 wie in comdare_add_test (gtest_setup.cmake:90): dieselbe Schranke fuer dieselbe Bauform.
add_test(NAME test_commands COMMAND test_commands)
add_test(NAME test_engine_adapters COMMAND test_engine_adapters)
set_tests_properties(test_commands test_engine_adapters PROPERTIES TIMEOUT 60)

# W-1 (2026-08-08), ZWEITE HAELFTE DES BEFUNDS -- die #278-"Not-Run"-Klasse:
# Bis zum 08.08.2026 waren diese beiden Ziele fuer ctest unsichtbar (enable_testing() stand in
# der Wurzel NACH add_subdirectory(libs/cache_engine); s. Wurzel-CMakeLists an der geheilten
# Stelle). Mit der Heilung wurden ihre gtest-Faelle sichtbar -- und damit fiel auf, dass das
# Sammel-Target comdare_tests sie NICHT baut: es haengt ausschliesslich an der globalen Liste
# COMDARE_TEST_TARGETS, und diese beiden standen dort nie. Am Objekt gemessen
# (08.08.2026, `ninja -t query comdare_tests`): 0 Kanten auf test_commands/test_engine_adapters.
# Die CI faehrt heute 'make' (all) und waere damit zufaellig gedeckt -- 'all' enthaelt beide
# Ziele. Auf Zufall beruht seither nichts an dieser Kette: die Registrierung macht die
# Deckung zur Eigenschaft des Bauwegs statt zur Eigenschaft der gerade gewaehlten
# Job-Zeile (die am 08.08. schon einmal von comdare_tests auf 'all' umgestellt wurde).
#
# NACHTRAG D2-G1 (10.08.2026), zwei Korrekturen am obigen Stand:
#   (a) Die Fallzahl ist 30, nicht 27. "20 + 7" war der Stand von 759b695a (06.07.2026); am
#       09.08. kamen mit cc9c233e/905bd1aa drei Welch-Faelle dazu. Heute per Bau und
#       --gtest_list_tests ausgezaehlt: test_commands 23, test_engine_adapters 7.
#   (b) Die NOT_BUILT-Platzhalter, die dieser Absatz beschreibt, KANN es hier nicht mehr geben:
#       sie waren ein Erzeugnis von gtest_discover_tests, das oben durch add_test ersetzt ist.
#       Ein add_test-Eintrag traegt den Zielnamen unabhaengig davon, ob die Binary gebaut wurde.
# Beides haelt jetzt tests/unit/test_d2g1_ctest_registrierung.cpp fest, nicht mehr dieser Kommentar.
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_commands)
set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS test_engine_adapters)

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(test_commands)
COMDARE_set_default_warnings(test_engine_adapters)
