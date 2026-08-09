# =============================================================================
# comdare-cache-engine -- CLI-SMOKE-TREIBER (D1e, 2026-08-09)
# =============================================================================
# WOZU. Ein ctest-Treiber, der eine gebaute CLI faehrt und BEIDE Zusicherungen
# prueft, die ein Rauchtest braucht:
#
#     (1) die Binary endet mit Exit-Code 0, UND
#     (2) ihre Ausgabe enthaelt ein bestimmtes Muster.
#
# WARUM ES DIESEN TREIBER GIBT -- die Falle, die er ersetzt. Der nahe liegende
# Weg ist die ctest-Eigenschaft PASS_REGULAR_EXPRESSION. Die kann das NICHT:
# ist sie gesetzt, entscheidet AUSSCHLIESSLICH der Regex-Treffer ueber
# Bestehen/Fallen -- der Exit-Code der Binary wird VOLLSTAENDIG verworfen. Eine
# CLI, die ihre Erfolgszeile druckt und danach mit Exit 1 abbricht, gilt dann
# als bestanden. Am Objekt nachgestellt (2026-08-09, CMake 3.31.6), vier Faelle,
# gleiches Werkzeug, einziger Unterschied die Eigenschaft:
#
#     Regex trifft + Exit!=0 + MIT  Eigenschaft ......... Passed   <-- der Riss
#     Regex trifft + Exit!=0 + OHNE Eigenschaft ......... Failed
#     Regex trifft + Exit==0 + MIT  Eigenschaft ......... Passed
#     Regex trifft + Exit==0 + OHNE Eigenschaft ......... Passed
#
# Der Riss ist also nicht theoretisch: Zeile 1 und 2 unterscheiden sich NUR in
# der Eigenschaft und fallen entgegengesetzt aus. Bei f15_compare_cli_smoke
# verdeckte das 11 von 12 numerischen return-Anweisungen in
# apps/f15_compare/main.cpp -- und DREI davon (return 4 in 549/558/566) liegen
# HINTER der geprueften Ranking-Zeile (481), sind also fuer den alten Test
# vollstaendig unsichtbar gewesen. Am Objekt nachgestellt mit einem
# unbeschreibbaren --csv-Pfad: Exit 4, Ranking-Zeile gedruckt, ctest "Passed".
#
# WARUM NICHT EINFACH DIE EIGENSCHAFT ENTFERNEN. Dann bindet zwar der
# Exit-Code, aber die Ausgabe wird gar nicht mehr geprueft -- und "Exit 0" ist
# laut TDD-Vertrag T-2 KEINE Zusicherung (Anwesenheit statt Aussage). Gefordert
# sind BEIDE Haelften, und genau die liefert dieser Treiber.
#
# ABGRENZUNG -- wo PASS_REGULAR_EXPRESSION RICHTIG bleibt: bei den drei
# Negativ-Compile-Fixtures (test_e24_c1_organ_concept_negativ,
# test_e24_c4_dock_version_negativ, test_hy_a1_reroute_gate_negativ) MUSS der
# Bau fehlschlagen. Dort ist das Verwerfen des Exit-Codes der Zweck, und der
# Marker beweist den BESTIMMTEN Bruch statt irgendeines. Dieser Treiber ist
# ausdruecklich NICHT fuer solche Fixtures gedacht -- er ist fuer den
# umgekehrten Fall: eine CLI, die GELINGEN muss.
#
# Erwartete -D Variablen:
#   CLI       gebaute Binary (ueber $<TARGET_FILE:...> uebergeben)
#   EXPECT    Regex, das in stdout+stderr vorkommen MUSS
# Optional:
#   CLI_ARGS  Argumente, als CMake-Liste (";"-getrennt)
#   LABEL     Klartextname fuer die Meldungen (Vorgabe: Dateiname der CLI)
#
# ASCII-only (Leitplanke, scripts/ci_diff_ascii_width_guard.sh).
# =============================================================================

foreach(_req CLI EXPECT)
    if(NOT DEFINED ${_req})
        message(FATAL_ERROR "cli_smoke: -D${_req} fehlt.")
    endif()
endforeach()

if(NOT DEFINED LABEL OR LABEL STREQUAL "")
    get_filename_component(LABEL "${CLI}" NAME)
endif()

# Nicht gebaut ist ein FEHLER, kein Ueberspringen: der Test ist nur registriert,
# wenn das Target existiert -- fehlt dann die Datei, ist die Baukante kaputt.
if(NOT EXISTS "${CLI}")
    message(FATAL_ERROR
        "cli_smoke [${LABEL}]: Binary nicht gebaut: ${CLI}\n"
        "Fehlt die Kante zum Sammel-Target comdare_tests? (COMDARE_TEST_TARGETS)")
endif()

execute_process(
    COMMAND "${CLI}" ${CLI_ARGS}
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE  _err)

set(_beide "${_out}${_err}")

# (1) EXIT-CODE BINDET. Das ist die Haelfte, die PASS_REGULAR_EXPRESSION verwarf.
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR
        "cli_smoke [${LABEL}]: EXIT-CODE ${_rc} (erwartet 0).\n"
        "  Aufruf: ${CLI} ${CLI_ARGS}\n"
        "--- Ausgabe ---\n${_beide}")
endif()

# (2) AUSSAGE STATT ANWESENHEIT (T-2). Exit 0 allein sichert nichts zu.
string(REGEX MATCH "${EXPECT}" _treffer "${_beide}")
if(_treffer STREQUAL "")
    message(FATAL_ERROR
        "cli_smoke [${LABEL}]: Exit 0, aber das erwartete Muster fehlt in der Ausgabe.\n"
        "  erwartet (Regex): ${EXPECT}\n"
        "  Aufruf: ${CLI} ${CLI_ARGS}\n"
        "--- Ausgabe ---\n${_beide}")
endif()

message(STATUS "cli_smoke [${LABEL}]: Exit 0 UND Muster '${EXPECT}' gefunden.")
