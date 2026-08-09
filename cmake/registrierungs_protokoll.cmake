# =============================================================================
# comdare-cache-engine -- REGISTRIERUNGS-PROTOKOLL (D2, 2026-08-09)
# =============================================================================
# WOZU. Die Abdeckungs-Wache (scripts/ci_test_coverage_guard.sh) rechnet die
# Vereinigung aller CI-Job-Auswahlen gegen die LIVE-Inventur 'ctest -N'. Diese
# Inventur ist ihr NENNER. Sie ist aber nicht konstant: mehrere Test-Registrier-
# ungen dieses Projekts haengen an einem 2-Pass-Werkzeug (Codegen-CLI, Adhoc-
# Emitter). Ist das Werkzeug im Bau-Baum nicht vorhanden, meldet die zustaendige
# CMake-Funktion "SKIPPED", der umgebende if()-Block wird uebersprungen -- und
# die darin registrierten Tests existieren in 'ctest -N' GAR NICHT.
#
# Die Wache sah davon bisher nichts. Sie rechnete korrekt -- ueber einem zu
# kleinen Gegenstand. AM OBJEKT GEMESSEN (2026-08-09, build-covguard in
# wt-ce-gnu; Verfahren: die fertig gebaute Werkzeug-Binary beiseitelegen,
# 'cmake -S . -B build-covguard' erneut laufen lassen, 'ctest -N' zaehlen --
# kein Neubau, nur Configure):
#   beide Werkzeuge vorhanden ....................... 434 Tests
#   Adhoc-Emitter beiseite, Codegen vorhanden ....... 432 Tests
#   beide beiseite (= Zustand nach Pass 1) .......... 430 Tests
# Die vier Fehlenden sind test_v41_anatomy_r5i_configure_codegen,
# test_v41_anatomy_f15_measurement, test_v41_anatomy_adhoc_autobuilt_load und
# f15_compare_cli_smoke. Auf ALLEN DREI Baeumen meldete die Wache in ihrem
# Stand vor D2 GRUEN (Exit 0) -- und hatte ueber die Menge, die sie sah, sogar
# recht. Genau das ist die Fehlerklasse: ein falsches Messgeraet faellt
# irgendwann auf, ein richtiges Messgeraet am falschen Gegenstand nie.
#
# WAS DIESE DATEI TUT. Jeder bedingte Registrierungs-Block vermerkt hier SELBST,
# ob er gelaufen ist und welche Tests er registriert haette. Das Protokoll landet
# als Datei im Bau-Baum; die Wache liest es und macht einen uebersprungenen Block
# ROT. Damit weiss sie, was sie NICHT sieht -- die Auskunft kommt vom Block
# selbst, nicht aus einer zweiten Abschrift in der Wache (dieselbe Doktrin wie
# scripts/ci_test_coverage_manifest.sh: Job und Wache lesen dasselbe Objekt).
#
# WARUM DIE TEST-NAMEN MITGESCHRIEBEN WERDEN. Dann kann die Wache auch die
# Gegenrichtung pruefen: ein Block, der sich AKTIV meldet, dessen Tests aber
# nicht in 'ctest -N' stehen, ist ebenso ein Befund (Umbenennung, verschluckte
# Registrierung). Ohne die Namen waere "AKTIV" eine unpruefbare Behauptung.
#
# ABLAGEORT: ${CMAKE_BINARY_DIR}, NICHT PROJECT_BINARY_DIR. Die Wache bekommt
# den ctest-Wurzelbaum uebergeben (dort liegt CTestTestfile.cmake) -- das ist
# CMAKE_BINARY_DIR. Im super-Sub-Build zeigt PROJECT_BINARY_DIR auf den
# ce-Unterbaum und das Protokoll laege neben dem falschen ctest-Wurzelknoten.
#
# ASCII-only (Leitplanke). Keine Umlaute, kein UTF-8.
# =============================================================================

set(COMDARE_REGISTRIERUNGS_PROTOKOLL "${CMAKE_BINARY_DIR}/comdare_registrierungs_protokoll.txt"
    CACHE INTERNAL "Pfad des Registrierungs-Protokolls fuer die Abdeckungs-Wache")

# -----------------------------------------------------------------------------
# _comdare_registrierung_init -- leert das Protokoll EINMAL je Configure-Lauf.
#
# Die globale Property lebt genau einen Configure-Lauf lang; ein Reconfigure
# beginnt also wieder bei null. Kein Anhaengen an den Lauf von gestern, und
# keine Reihenfolge-Abhaengigkeit zwischen den aufrufenden Dateien: wer zuerst
# vermerkt, legt an.
# -----------------------------------------------------------------------------
function(_comdare_registrierung_init)
    get_property(_init GLOBAL PROPERTY COMDARE_REGISTRIERUNG_INIT SET)
    if(_init)
        return()
    endif()
    set_property(GLOBAL PROPERTY COMDARE_REGISTRIERUNG_INIT 1)
    set_property(GLOBAL PROPERTY COMDARE_REGISTRIERUNG_ANZAHL 0)
    file(WRITE "${COMDARE_REGISTRIERUNGS_PROTOKOLL}"
        "# comdare-cache-engine -- Registrierungs-Protokoll der bedingten Test-Bloecke.\n"
        "# Erzeugt zur Configure-Zeit von cmake/registrierungs_protokoll.cmake.\n"
        "# Gelesen von scripts/ci_test_coverage_guard.sh (NENNER der Abdeckungs-Wache).\n"
        "# Format: BLOCK|<kennung>|AKTIV|UEBERSPRUNGEN|<tests,komma>|<grund>\n")
endfunction()

# -----------------------------------------------------------------------------
# comdare_registrierung_vermerken -- ein bedingter Registrierungs-Block meldet
# sich. Beide Zweige des if() muessen vermerken, sonst ist der Block fuer die
# Wache unsichtbar (und das faellt auf: die Abschluss-Zeile zaehlt mit).
#
#   BLOCK  <kennung>              stabile Kennung, erscheint in der Wachen-Ausgabe
#   STATUS AKTIV|UEBERSPRUNGEN    hat der Block registriert oder nicht
#   [TESTS <name>...]             die Tests, die dieser Block registriert (haette)
#   [GRUND <text>]                Klartext fuer die Wachen-Ausgabe
# -----------------------------------------------------------------------------
function(comdare_registrierung_vermerken)
    set(_options)
    set(_one_value BLOCK STATUS GRUND)
    set(_multi_value TESTS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT ARG_BLOCK)
        message(FATAL_ERROR "comdare_registrierung_vermerken: BLOCK ist Pflicht.")
    endif()
    if(NOT ARG_STATUS MATCHES "^(AKTIV|UEBERSPRUNGEN)$")
        message(FATAL_ERROR
            "comdare_registrierung_vermerken(${ARG_BLOCK}): STATUS muss AKTIV oder "
            "UEBERSPRUNGEN sein, war '${ARG_STATUS}'.")
    endif()

    _comdare_registrierung_init()

    # Ein bedingter Block darf sich nicht zweimal melden -- sonst zaehlt die
    # Wache denselben Gegenstand doppelt und ein zweiter, echter Block koennte
    # unbemerkt fehlen.
    get_property(_bekannt GLOBAL PROPERTY COMDARE_REGISTRIERUNG_KENNUNGEN)
    if("${ARG_BLOCK}" IN_LIST _bekannt)
        message(FATAL_ERROR
            "comdare_registrierung_vermerken: Kennung '${ARG_BLOCK}' wurde in diesem "
            "Configure-Lauf bereits vermerkt. Kennungen muessen eindeutig sein.")
    endif()
    set_property(GLOBAL APPEND PROPERTY COMDARE_REGISTRIERUNG_KENNUNGEN "${ARG_BLOCK}")

    set(_tests "-")
    if(ARG_TESTS)
        string(REPLACE ";" "," _tests "${ARG_TESTS}")
    endif()

    set(_grund "${ARG_GRUND}")
    # Das Trennzeichen und Zeilenumbrueche duerfen nicht im Freitext landen,
    # sonst zerfaellt die Zeile beim Lesen in zwei Datensaetze.
    string(REPLACE "|" "/" _grund "${_grund}")
    string(REPLACE "\n" " " _grund "${_grund}")
    string(REPLACE "\r" " " _grund "${_grund}")

    file(APPEND "${COMDARE_REGISTRIERUNGS_PROTOKOLL}"
        "BLOCK|${ARG_BLOCK}|${ARG_STATUS}|${_tests}|${_grund}\n")

    get_property(_n GLOBAL PROPERTY COMDARE_REGISTRIERUNG_ANZAHL)
    math(EXPR _n "${_n} + 1")
    set_property(GLOBAL PROPERTY COMDARE_REGISTRIERUNG_ANZAHL "${_n}")
endfunction()

# -----------------------------------------------------------------------------
# comdare_registrierung_abschliessen -- schreibt die Abschluss-Zeile.
#
# WOZU EINE ABSCHLUSS-ZEILE. Ein abgebrochener Configure-Lauf hinterlaesst ein
# HALBES Protokoll. Ohne Endmarke saehe die Wache einfach weniger Bloecke und
# faende daran nichts Auffaelliges -- exakt der stille Nenner-Schwund, gegen den
# diese Datei gebaut ist. Mit Endmarke gilt: fehlt sie oder passt die Zahl
# nicht, bricht die Wache ab (fail-closed). Wird nach dem Abschluss noch ein
# Block vermerkt, stimmt die Zahl ebenfalls nicht mehr -- auch das ist laut.
# Aufgerufen wird sie am Ende der Wurzel-CMakeLists.txt, also nach allem, was
# add_subdirectory() konfiguriert hat.
# -----------------------------------------------------------------------------
function(comdare_registrierung_abschliessen)
    _comdare_registrierung_init()
    get_property(_n GLOBAL PROPERTY COMDARE_REGISTRIERUNG_ANZAHL)
    file(APPEND "${COMDARE_REGISTRIERUNGS_PROTOKOLL}" "ENDE|${_n}\n")
    message(STATUS
        "Registrierungs-Protokoll: ${_n} bedingte Registrierungs-Bloecke vermerkt "
        "-> ${COMDARE_REGISTRIERUNGS_PROTOKOLL}")
endfunction()
