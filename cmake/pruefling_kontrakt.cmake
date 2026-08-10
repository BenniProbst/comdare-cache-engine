# =============================================================================
#  PRUEFLING-KONTRAKT -- ZUSICHERUNG STATT ANWESENHEIT
#  (W0a/S2, 2026-08-10)
# =============================================================================
#
# ANWESENHEIT DARF NUR WIDERLEGEN, NIE ERLAUBEN.
# Das EXISTS ueber die BELEG-Header in diesem Modul erzeugt ausschliesslich ROT.
# Es kann kein Gatter oeffnen. Geoeffnet wird ausschliesslich durch eine
# Zusicherung -- eine Aussage der Partei, die sie belegen kann.
#
# DER BEFUND, GEGEN DEN DIESES MODUL GEBAUT IST (am Objekt gemessen, 10.08.2026,
# ce 9f932e91 gegen comdare-prt-art c6f0754):
#   tests/unit/CMakeLists.txt gatterte vier Tests mit
#       if(EXISTS "${PROJECT_SOURCE_DIR}/prt_art/include/prt_art/prt_art.hpp")
#   Diese Datei existiert seit dem 14.05.2026 nirgends -- und der Plugin-Weg
#   (COMDARE_CE_PRUEFLINGE) haengt Pruefling-Header ausschliesslich ueber
#   COMDARE_PRUEFLING_INCLUDE_DIRS an, kopiert NIE etwas unter die CE-Wurzel.
#   Das Gatter konnte per Konstruktion nie oeffnen. Gemessen mit geladenem
#   echtem Pruefling: die vier Tests erschienen in `ctest -N` 0 mal, der
#   Pruefling-Integrationstest 1 mal, Gegenprobe test_pressure_state 1 mal.
#
# WARUM EIN EXISTS HIER STRUKTURELL FALSCH IST (T-2):
#   Ein EXISTS beantwortet "liegt an dem Ort, den ICH mir denke, eine Datei?".
#   CE hat damit das Innenleben eines fremden Repos modelliert. Genau daran ist
#   es zerbrochen: der Pruefling darf seine Header verschieben. Die Frage, die
#   ein Gatter stellen muss, ist "WAS sichert der Pruefling zu?" -- und die kann
#   nur der Pruefling beantworten.
#
# WAS DIESES MODUL NICHT LEISTET -- ausdruecklich benannt:
#   Die Zusicherung ist eine Aussage ueber das ANGEBOT, nicht ueber die
#   UEBERSETZBARKEIT. Ein Pruefling, der pruefling_slots_v1 zusichert und dessen
#   Slots gegen die heutigen CE-Concepts nicht uebersetzen, oeffnet das Gatter
#   und bricht dann HART im Compiler. Das ist gewollt: ein harter Uebersetzungs-
#   fehler ist ein Befund, ein stilles FALSE ist keiner. Der Beweis der
#   Erfuellung liegt in C++ (static_assert), nicht in CMake.
#
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# DIE ABSCHLIESSENDE LISTE DER BEKANNTEN FAEHIGKEITEN.
# Abschliessend heisst: was hier nicht steht, wird NICHT geraten, sondern
# abgelehnt. Ein Tippfehler in einer Zusicherung ist sonst ein stilles FALSE --
# also exakt die Fehlerform, die dieses Modul beseitigt.
# -----------------------------------------------------------------------------
set(COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN
    "pruefling_slots_v1"    # Der Pruefling liefert Achsen-Slots nach
                            # libs/cache_engine/anatomy/pruefling_merge.hpp
                            # (PrueflingVariants + has_pruefling).
)

# -----------------------------------------------------------------------------
# comdare_pruefling_deklarieren(
#     NAME         <kennung>
#     FAEHIGKEITEN <f> [<f>...]
#     INCLUDE_DIRS <dir> [<dir>...]
#     [TEST_SOURCES <src> [<src>...]]
#     [BELEG        <relpfad> [<relpfad>...]])
#
# Wird aus <pruefling>/comdare_pruefling.cmake heraus aufgerufen. Laeuft im
# Scope der CE-Root-CMakeLists (include(), nicht add_subdirectory) -- daher
# propagieren die Listen per PARENT_SCOPE dorthin und weiter nach tests/unit.
# -----------------------------------------------------------------------------
function(comdare_pruefling_deklarieren)
    set(_options)
    set(_one_value NAME)
    set(_multi_value FAEHIGKEITEN INCLUDE_DIRS TEST_SOURCES BELEG)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT ARG_NAME)
        message(FATAL_ERROR "comdare_pruefling_deklarieren: NAME ist Pflicht.")
    endif()
    if(NOT ARG_FAEHIGKEITEN)
        message(FATAL_ERROR
            "comdare_pruefling_deklarieren(${ARG_NAME}): FAEHIGKEITEN ist Pflicht. "
            "Ein Pruefling, der nichts zusichert, ist keiner.")
    endif()
    if(NOT ARG_INCLUDE_DIRS)
        message(FATAL_ERROR
            "comdare_pruefling_deklarieren(${ARG_NAME}): INCLUDE_DIRS ist Pflicht -- "
            "ohne Include-Wurzel ist keine Zusicherung belegbar.")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "comdare_pruefling_deklarieren(${ARG_NAME}): unbekannte Argumente "
            "'${ARG_UNPARSED_ARGUMENTS}' -- wird nicht geraten.")
    endif()

    # REGEL 3 -- unbekannte Faehigkeit. Nicht raten, ablehnen.
    foreach(_f IN LISTS ARG_FAEHIGKEITEN)
        if(NOT "${_f}" IN_LIST COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN)
            message(FATAL_ERROR
                "comdare_pruefling_deklarieren(${ARG_NAME}): unbekannte Faehigkeit "
                "'${_f}'. Bekannt sind: ${COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN}. "
                "Eine unbekannte Zusicherung wird NICHT geraten.")
        endif()
    endforeach()

    # REGEL 5 -- zwei Pruefling duerfen dieselbe Faehigkeit nicht zusichern.
    # Sonst entschiede die Ladereihenfolge, wessen Slots gelten.
    get_property(_belegt GLOBAL PROPERTY COMDARE_PRUEFLING_FAEHIGKEITEN)
    foreach(_f IN LISTS ARG_FAEHIGKEITEN)
        if("${_f}" IN_LIST _belegt)
            get_property(_wer GLOBAL PROPERTY "COMDARE_PRUEFLING_FAEHIGKEIT_QUELLE_${_f}")
            message(FATAL_ERROR
                "comdare_pruefling_deklarieren(${ARG_NAME}): Faehigkeit '${_f}' wurde "
                "bereits von Pruefling '${_wer}' zugesichert. Die Ladereihenfolge darf "
                "nicht entscheiden, wessen Slots gelten.")
        endif()
    endforeach()

    # REGEL 4 -- der BELEG widerlegt. Loest ein zugesicherter Header unter KEINER
    # der deklarierten Include-Wurzeln auf, ist die Zusicherung nachweislich
    # falsch. Dieses EXISTS kann nur ROT erzeugen: es steht hinter der
    # Zusicherung, nie vor ihr, und oeffnet nichts.
    foreach(_b IN LISTS ARG_BELEG)
        set(_gefunden FALSE)
        foreach(_d IN LISTS ARG_INCLUDE_DIRS)
            if(EXISTS "${_d}/${_b}")
                set(_gefunden TRUE)
                break()
            endif()
        endforeach()
        if(NOT _gefunden)
            message(FATAL_ERROR
                "comdare_pruefling_deklarieren(${ARG_NAME}): BELEG-Header '${_b}' loest "
                "unter keiner der Include-Wurzeln auf (${ARG_INCLUDE_DIRS}). Die "
                "Zusicherung '${ARG_FAEHIGKEITEN}' ist damit widerlegt.")
        endif()
    endforeach()

    # Eintragen -- erst NACH allen Pruefungen, damit ein Abbruch keinen halben
    # Zustand hinterlaesst.
    foreach(_f IN LISTS ARG_FAEHIGKEITEN)
        set_property(GLOBAL APPEND PROPERTY COMDARE_PRUEFLING_FAEHIGKEITEN "${_f}")
        set_property(GLOBAL PROPERTY "COMDARE_PRUEFLING_FAEHIGKEIT_QUELLE_${_f}" "${ARG_NAME}")
    endforeach()
    set_property(GLOBAL APPEND PROPERTY COMDARE_PRUEFLING_NAMEN "${ARG_NAME}")

    # Der Zaehler traegt REGEL 2 (Snippet ohne Deklaration) im Loader.
    get_property(_n GLOBAL PROPERTY COMDARE_PRUEFLING_DEKLARATIONEN)
    if(NOT _n)
        set(_n 0)
    endif()
    math(EXPR _n "${_n} + 1")
    set_property(GLOBAL PROPERTY COMDARE_PRUEFLING_DEKLARATIONEN "${_n}")

    # Rueckwaertskompatibel: die beiden Listen, die tests/unit/CMakeLists.txt
    # bereits konsumiert, werden weiter gefuellt. Der Konsument aendert sich
    # nicht -- nur die Frage, die das Gatter stellt.
    set(COMDARE_PRUEFLING_INCLUDE_DIRS
        ${COMDARE_PRUEFLING_INCLUDE_DIRS} ${ARG_INCLUDE_DIRS} PARENT_SCOPE)
    set(COMDARE_PRUEFLING_TEST_SOURCES
        ${COMDARE_PRUEFLING_TEST_SOURCES} ${ARG_TEST_SOURCES} PARENT_SCOPE)

    message(STATUS
        "comdare Pruefling '${ARG_NAME}' sichert zu: ${ARG_FAEHIGKEITEN}")
endfunction()

# -----------------------------------------------------------------------------
# comdare_pruefling_faehigkeit(<faehigkeit> <ausgabe-variable>)
#
# DIE FRAGE, DIE EIN GATTER STELLEN DARF. Sie fragt nach einer AUSSAGE, nicht
# nach einem Pfad. Sie kennt kein Verzeichnis-Layout eines fremden Repos.
# -----------------------------------------------------------------------------
function(comdare_pruefling_faehigkeit _faehigkeit _out)
    if(NOT "${_faehigkeit}" IN_LIST COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN)
        message(FATAL_ERROR
            "comdare_pruefling_faehigkeit: nach unbekannter Faehigkeit '${_faehigkeit}' "
            "gefragt. Bekannt: ${COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN}. Eine Abfrage, "
            "die immer FALSE liefert, ist ein stilles Gatter -- genau der Defekt.")
    endif()
    get_property(_belegt GLOBAL PROPERTY COMDARE_PRUEFLING_FAEHIGKEITEN)
    if("${_faehigkeit}" IN_LIST _belegt)
        set(${_out} TRUE PARENT_SCOPE)
    else()
        set(${_out} FALSE PARENT_SCOPE)
    endif()
endfunction()

# -----------------------------------------------------------------------------
# comdare_pruefling_ort_kreuzprobe()  -- S7: die ZWEI Wahrheiten ueber den Ort
#
# COMDARE_CE_PRUEFLINGE  ist der CODE-Weg   (Plugin-Controller, Root-CMakeLists)
# COMDARE_PRT_ART_DIR    ist der REGISTRY-Weg (XML fuer den Planer, aus super)
# Nichts verband die beiden. Jede kann ohne die andere gesetzt sein; dann laufen
# Code-Haelfte und Registry-Haelfte der Pruefling-Integration still auseinander.
# -----------------------------------------------------------------------------
function(comdare_pruefling_ort_kreuzprobe)
    if(NOT COMDARE_PRT_ART_DIR AND NOT COMDARE_CE_PRUEFLINGE)
        return()
    endif()

    if(COMDARE_PRT_ART_DIR AND NOT COMDARE_CE_PRUEFLINGE)
        message(STATUS
            "comdare Pruefling-Ort: COMDARE_PRT_ART_DIR ist gesetzt "
            "('${COMDARE_PRT_ART_DIR}'), COMDARE_CE_PRUEFLINGE ist LEER -- die "
            "XML-Registry ist da, der Code-Weg nicht. Kein Pruefling-Slot wird gebaut.")
        return()
    endif()
    if(COMDARE_CE_PRUEFLINGE AND NOT COMDARE_PRT_ART_DIR)
        message(STATUS
            "comdare Pruefling-Ort: COMDARE_CE_PRUEFLINGE ist gesetzt, "
            "COMDARE_PRT_ART_DIR ist LEER -- der Code-Weg ist da, die XML-Registry "
            "nicht. Der Planer sieht die Pruefling-Achsen nicht.")
        return()
    endif()

    get_filename_component(_registry_real "${COMDARE_PRT_ART_DIR}" REALPATH)
    set(_treffer FALSE)
    foreach(_d IN LISTS COMDARE_CE_PRUEFLINGE)
        get_filename_component(_code_real "${_d}" REALPATH)
        if("${_code_real}" STREQUAL "${_registry_real}")
            set(_treffer TRUE)
            break()
        endif()
    endforeach()
    if(NOT _treffer)
        message(FATAL_ERROR
            "comdare Pruefling-Ort: die beiden Wahrheiten zeigen auf verschiedene Orte. "
            "COMDARE_PRT_ART_DIR (Registry) -> '${_registry_real}'; "
            "COMDARE_CE_PRUEFLINGE (Code) -> '${COMDARE_CE_PRUEFLINGE}'. "
            "Code-Haelfte und Registry-Haelfte wuerden still auseinanderlaufen.")
    endif()
    message(STATUS "comdare Pruefling-Ort: Code- und Registry-Weg zeigen auf '${_registry_real}'.")
endfunction()
