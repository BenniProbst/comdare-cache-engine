# =============================================================================
# comdare-cache-engine -- OS-2-OPTFLAG-GATE (VO3-1(b), Owner-Antwort OS-2 26.08.2026)
# =============================================================================
# WOZU. VO3-1(b) (Owner-KERN X4 25.08.2026) setzt die Release-Opt-Stufe GLOBAL
# per CACHE-FORCE (cmake/compiler_flags.cmake). Der Explore-Befund davor: 351
# von 643 messwirksamen TUs bauten -O3 an der Achse VORBEI, weil Vendor-/
# FetchContent-/Direktblock-Code eigene Flags setzte. Dieses Gate macht den
# Rueckfall UNMOEGLICH statt unwahrscheinlich: der Configure bricht FATAL,
# sobald irgendeine TU des Baums eigene Release-Opt-Flags traegt, die NICHT der
# VO3-1(b)-Unterstellung (global ${_COMDARE_release_opt}) folgen.
#
# DREI FANGNETZE (alle drei am Objekt per Probe bewiesen, /tmp-Probeprojekte
# 26.08. + tests/unit/vo31_optflag_gate_probe.cmake):
#   (1) variable_watch auf CMAKE_{C,CXX}_FLAGS{,_RELEASE}: faengt NORMALE
#       Variablen-Schatten in Unterverzeichnissen (Vendor-Muster; exakt der
#       Reichweiten-Riss der design/probes/ B/C -- die Cache luegt dann).
#   (2) End-of-Configure-Scan (cmake_language DEFER an CMAKE_SOURCE_DIR --
#       deckt im super-Embed auch die super-Ziele): Directory-/Target-/Quell-
#       COMPILE_OPTIONS, COMPILE_FLAGS, INTERFACE_COMPILE_OPTIONS.
#   (3) Install-Probe der schon gesetzten CMAKE_{C,CXX}_FLAGS (Kommandozeilen-
#       bzw. Cache-Injektion VOR dem Watch).
# Genex-Semantik: Tokens unter $<$<CONFIG:Debug|RelWithDebInfo|MinSizeRel>:...>
# sind NICHT Release-wirksam und werden uebersprungen; alles andere zaehlt
# (fail-closed: ein unzerlegbarer Genex-Rest wird gescannt, nie verworfen).
#
# AUSNAHMEN sind EINGEFROREN und ZWEISEITIG deklariert: der Eintrag unten UND
# der Weg ueber comdare_apply_optimization_level_flags() (isa_features.cmake,
# traegt in COMDARE_VO31_OPTFLAG_DEKLARIERT ein). Einer ohne den anderen ist
# ein FATAL -- eine Ausnahme, die nur an einer Stelle steht, ist keine.
#
# SELBSTTEST: der Token-Kern wird bei JEDEM Install gegen 9 eingefrorene
# Fixtures gefahren (beisst er nicht, ist jedes Gruen wertlos -> FATAL).
# KOEDER-NEGATIVPROBE: -DCOMDARE_VO31_OPTFLAG_GATE_KOEDER=ON pflanzt ein
# -O3-Ziel in den ECHTEN Baum -- der Configure MUSS dann rot sein (T-1;
# dauerhafte Probe: tests/unit/vo31_optflag_gate_probe.cmake, 3 Koeder-Klassen).
#
# ASCII-only (Leitplanke). MSVC: Gate nicht installiert (kein -O-Token-Raum;
# /O2-Klasse ist CMake-Vorgabe, s. compiler_flags.cmake).
# =============================================================================

# Eingefrorene Ausnahme-Tafel: "<target>=<flag>" je Eintrag, mit Grund.
#   test_all19_segment_timer=-O3  mess-charakteristisch (Layout-Sensitivitaet nur
#       UNTER Optimierung > 5% sichtbar; tests/unit/CMakeLists.txt, R-4 des
#       BAUPLAN-VO3-1-GLOBAL-O2: NICHT an COMDARE_OPT_O3 koppeln).
set(COMDARE_VO31_OPTFLAG_AUSNAHMEN "test_all19_segment_timer=-O3"
    CACHE INTERNAL "OS-2: eingefrorene Opt-Flag-Ausnahmen (target=flag)")

option(COMDARE_VO31_OPTFLAG_GATE_KOEDER
    "OS-2-Negativprobe: pflanzt ein -O3-Koeder-Ziel -- der Configure MUSS rot enden" OFF)

# -- Token-Kern: Release-wirksame -O-Tokens einer Property-/Variablen-Zeichenkette --------------------
function(_comdare_vo31_scan_string eingabe soll nur_release outvar)
    set(_s "${eingabe}")
    if(nur_release)
        # NICHT-Release-Configs abwerfen (Debug/RelWithDebInfo/MinSizeRel-Genex).
        string(REGEX REPLACE "\\$<\\$<CONFIG:(Debug|RelWithDebInfo|MinSizeRel)>:[^>]*>" " " _s "${_s}")
    endif()
    # Genex-/Listen-Struktur zu Wortgrenzen verflachen, dann tokenisieren.
    string(REGEX REPLACE "[$<>:;,\"']" " " _s "${_s}")
    string(REGEX MATCHALL "[^ \t\r\n]+" _toks "${_s}")
    set(_bad "")
    foreach(_t IN LISTS _toks)
        if(_t MATCHES "^-O(fast|[0-9sgz])?$" AND NOT _t STREQUAL "${soll}")
            list(APPEND _bad "${_t}")
        endif()
    endforeach()
    set(${outvar} "${_bad}" PARENT_SCOPE)
endfunction()

# -- Selbsttest des Kerns (9 Fixtures; FATAL wenn der Kern nicht beisst oder falsch beisst) -----------
function(_comdare_vo31_gate_selbsttest soll)
    set(_faelle
        "-O2|0" "-O3|1" "$<$<CONFIG:Release>:-O3>|1" "$<$<CONFIG:Debug>:-O0 -g3>|0"
        "$<$<CONFIG:RelWithDebInfo>:-O2 -g>|0" "-Wno-Override|0" "-Ofast|1" "-O|1" "-O2 -O3|1")
    foreach(_f IN LISTS _faelle)
        string(REPLACE "|" ";" _p "${_f}")
        list(GET _p 0 _in)
        list(GET _p 1 _sollzahl)
        _comdare_vo31_scan_string("${_in}" "${soll}" TRUE _bad)
        list(LENGTH _bad _n)
        if(NOT _n EQUAL _sollzahl)
            message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT (SELBSTTEST): Fixture '${_in}' ergab ${_n} "
                "Verstoesse, erwartet ${_sollzahl} -- der Token-Kern ist stumpf, jedes Gruen waere wertlos.")
        endif()
    endforeach()
endfunction()

# -- Fangnetz (1): variable_watch-Callback ------------------------------------------------------------
function(_comdare_vo31_optflag_watch var access value file stack)
    if(NOT access STREQUAL "MODIFIED_ACCESS")
        return()
    endif()
    get_property(_soll GLOBAL PROPERTY COMDARE_VO31_OPTFLAG_SOLL)
    if(var MATCHES "_FLAGS_RELEASE$")
        _comdare_vo31_scan_string("${value}" "${_soll}" TRUE _bad)
        _comdare_vo31_scan_string("${value}" "__nie__" TRUE _alle)
        list(LENGTH _alle _n_alle)
        if(_bad OR _n_alle EQUAL 0)
            message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT (WACHE): ${var} wird zu '${value}' gesetzt "
                "(Quelle: ${file}) -- fremde Tokens [${_bad}] bzw. kein Opt-Token (effektiv -O0). "
                "Die Release-Stufe ist GLOBAL ${_soll} (Owner-KERN X4 25.08.2026); der offizielle "
                "O3-Weg ist COMDARE_OPT_O3=ON, der Tier-Weg die XML-opt_level-Unterachse.")
        endif()
    else()
        _comdare_vo31_scan_string("${value}" "__nie__" FALSE _bad)
        if(_bad)
            message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT (WACHE): ${var} bekommt Opt-Token(s) [${_bad}] "
                "(Quelle: ${file}, Wert '${value}') -- Opt-Stufen reisen NIE in den All-Config-Flags "
                "(sie draengeln sich sonst still vor die Config-Stufen aller Bautypen).")
        endif()
    endif()
endfunction()

# -- Fangnetz (2): End-of-Configure-Scan --------------------------------------------------------------
function(comdare_vo31_optflag_gate_run phase)
    get_property(_soll GLOBAL PROPERTY COMDARE_VO31_OPTFLAG_SOLL)
    get_property(_deklariert GLOBAL PROPERTY COMDARE_VO31_OPTFLAG_DEKLARIERT)
    if(NOT _deklariert)
        set(_deklariert "")
    endif()
    # Deklarations-Kreuzprobe (Seite 1): jede per Funktion deklarierte fremde Stufe muss allowlisted sein.
    foreach(_d IN LISTS _deklariert)
        string(REPLACE "=" ";" _dp "${_d}")
        list(GET _dp 1 _dflag)
        if(NOT _dflag STREQUAL "${_soll}" AND NOT "${_d}" IN_LIST COMDARE_VO31_OPTFLAG_AUSNAHMEN)
            message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT [${phase}]: '${_d}' ist per "
                "comdare_apply_optimization_level_flags deklariert, steht aber NICHT in der "
                "eingefrorenen Allowlist (cmake/vo31_optflag_gate.cmake) -- beide Seiten sind Pflicht.")
        endif()
    endforeach()
    set(_dirs "${CMAKE_SOURCE_DIR}")
    set(_i 0)
    set(_n_t 0)
    set(_n_s 0)
    set(_verstoesse "")
    set(_gedeckt "")
    while(TRUE)
        list(LENGTH _dirs _n)
        if(NOT _i LESS _n)
            break()
        endif()
        list(GET _dirs ${_i} _d)
        math(EXPR _i "${_i}+1")
        get_directory_property(_subs DIRECTORY "${_d}" SUBDIRECTORIES)
        if(_subs)
            list(APPEND _dirs ${_subs})
        endif()
        get_directory_property(_dco DIRECTORY "${_d}" COMPILE_OPTIONS)
        if(_dco)
            _comdare_vo31_scan_string("${_dco}" "${_soll}" TRUE _bad)
            if(_bad)
                list(APPEND _verstoesse "VERZEICHNIS ${_d}: [${_bad}]")
            endif()
        endif()
        get_directory_property(_tg DIRECTORY "${_d}" BUILDSYSTEM_TARGETS)
        foreach(_t IN LISTS _tg)
            get_target_property(_ty ${_t} TYPE)
            if(_ty STREQUAL "UTILITY")
                continue()
            endif()
            math(EXPR _n_t "${_n_t}+1")
            get_target_property(_ico ${_t} INTERFACE_COMPILE_OPTIONS)
            if(_ico)
                _comdare_vo31_scan_string("${_ico}" "${_soll}" TRUE _bad)
                if(_bad)
                    list(APPEND _verstoesse "ZIEL ${_t} (INTERFACE): [${_bad}]")
                endif()
            endif()
            if(_ty STREQUAL "INTERFACE_LIBRARY")
                continue()
            endif()
            set(_eigene "")
            get_target_property(_co ${_t} COMPILE_OPTIONS)
            if(_co)
                _comdare_vo31_scan_string("${_co}" "${_soll}" TRUE _bad)
                list(APPEND _eigene ${_bad})
            endif()
            get_target_property(_cf ${_t} COMPILE_FLAGS)
            if(_cf)
                _comdare_vo31_scan_string("${_cf}" "${_soll}" TRUE _bad)
                list(APPEND _eigene ${_bad})
            endif()
            if(_eigene)
                # Ausnahme-Pruefung: gedeckt NUR wenn (a) allowlisted UND (b) deklariert UND (c) exakt.
                set(_offen "")
                foreach(_b IN LISTS _eigene)
                    if("${_t}=${_b}" IN_LIST COMDARE_VO31_OPTFLAG_AUSNAHMEN
                       AND "${_t}=${_b}" IN_LIST _deklariert)
                        list(APPEND _gedeckt "${_t}=${_b}")
                    else()
                        list(APPEND _offen "${_b}")
                    endif()
                endforeach()
                if(_offen)
                    list(APPEND _verstoesse "ZIEL ${_t}: [${_offen}]")
                endif()
            endif()
            get_target_property(_sd ${_t} SOURCE_DIR)
            get_target_property(_src ${_t} SOURCES)
            if(NOT _src)
                continue()
            endif()
            foreach(_s IN LISTS _src)
                if(_s MATCHES "^\\$<")
                    continue()
                endif()
                if(NOT IS_ABSOLUTE "${_s}")
                    set(_s "${_sd}/${_s}")
                endif()
                math(EXPR _n_s "${_n_s}+1")
                get_source_file_property(_sco "${_s}" TARGET_DIRECTORY ${_t} COMPILE_OPTIONS)
                get_source_file_property(_scf "${_s}" TARGET_DIRECTORY ${_t} COMPILE_FLAGS)
                foreach(_w IN ITEMS "${_sco}" "${_scf}")
                    if(_w AND NOT _w STREQUAL "NOTFOUND")
                        _comdare_vo31_scan_string("${_w}" "${_soll}" TRUE _bad)
                        if(_bad)
                            list(APPEND _verstoesse "QUELLE ${_s} (Ziel ${_t}): [${_bad}]")
                        endif()
                    endif()
                endforeach()
            endforeach()
        endforeach()
    endwhile()
    # Deklarations-Kreuzprobe (Seite 2): jeder Allowlist-Eintrag mit existierendem Ziel muss LEBEN
    # (Token vorhanden + deklariert) -- ein stale Eintrag ist eine falsche Zusicherung.
    set(_ausn_uebersprungen 0)
    foreach(_a IN LISTS COMDARE_VO31_OPTFLAG_AUSNAHMEN)
        string(REPLACE "=" ";" _ap "${_a}")
        list(GET _ap 0 _at)
        if(NOT TARGET ${_at})
            math(EXPR _ausn_uebersprungen "${_ausn_uebersprungen}+1")
            continue()
        endif()
        if(NOT "${_a}" IN_LIST _gedeckt)
            message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT [${phase}]: Allowlist-Eintrag '${_a}' ist STALE "
                "(Ziel existiert, traegt das Flag aber nicht oder nicht ueber "
                "comdare_apply_optimization_level_flags) -- Eintrag entfernen oder Deklaration heilen.")
        endif()
    endforeach()
    list(LENGTH _verstoesse _n_v)
    if(_n_v GREATER 0)
        list(JOIN _verstoesse "\n  " _vtext)
        message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT [${phase}]: ${_n_v} Verstoss/Verstoesse gegen die "
            "globale Release-Stufe ${_soll} (Owner-KERN X4 25.08.2026 + OS-2 26.08.2026):\n  ${_vtext}\n"
            "Der offizielle O3-Weg ist COMDARE_OPT_O3=ON (haus-weit, unter WARNING); der Tier-Weg ist "
            "die XML-opt_level-Unterachse. Ausnahmen NUR ueber comdare_apply_optimization_level_flags "
            "PLUS Allowlist (cmake/vo31_optflag_gate.cmake).")
    endif()
    list(REMOVE_DUPLICATES _gedeckt)
    list(LENGTH _gedeckt _n_g)
    message(STATUS "VO3-1(b) OS-2-GATE GRUEN [${phase}]: soll=${_soll}, ${_n_t} Ziele / ${_n_s} Quellen "
        "geprueft, 0 Verstoesse; Ausnahmen gedeckt: ${_n_g} [${_gedeckt}], ohne Ziel uebersprungen: "
        "${_ausn_uebersprungen}.")
endfunction()

# -- Installation (aus cmake/compiler_flags.cmake, non-MSVC) ------------------------------------------
function(comdare_vo31_optflag_gate_install soll)
    get_property(_da GLOBAL PROPERTY COMDARE_VO31_OPTFLAG_GATE_INSTALLIERT SET)
    if(_da)
        return()
    endif()
    set_property(GLOBAL PROPERTY COMDARE_VO31_OPTFLAG_GATE_INSTALLIERT 1)
    set_property(GLOBAL PROPERTY COMDARE_VO31_OPTFLAG_SOLL "${soll}")
    _comdare_vo31_gate_selbsttest("${soll}")
    # Fangnetz (3): schon gesetzte All-Config-Flags (Kommandozeile/Cache) VOR dem Watch.
    foreach(_v CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        _comdare_vo31_scan_string("${${_v}}" "__nie__" FALSE _bad)
        if(_bad)
            message(FATAL_ERROR "VO3-1(b) OS-2-GATE ROT (INSTALL): ${_v}='${${_v}}' traegt Opt-Token(s) "
                "[${_bad}] -- Opt-Stufen reisen NIE in den All-Config-Flags (Owner-KERN X4/OS-2).")
        endif()
    endforeach()
    foreach(_v CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        variable_watch(${_v} _comdare_vo31_optflag_watch)
    endforeach()
    if(COMDARE_VO31_OPTFLAG_GATE_KOEDER)
        # T-1-Negativprobe am ECHTEN Baum: dieses Ziel MUSS den Lauf rot machen.
        file(WRITE "${CMAKE_BINARY_DIR}/vo31_gate_koeder/koeder.cpp" "int vo31_gate_koeder() { return 3; }\n")
        add_library(comdare_vo31_optflag_gate_koeder OBJECT EXCLUDE_FROM_ALL
            "${CMAKE_BINARY_DIR}/vo31_gate_koeder/koeder.cpp")
        target_compile_options(comdare_vo31_optflag_gate_koeder PRIVATE -O3)
        message(STATUS "OS-2: KOEDER gepflanzt (comdare_vo31_optflag_gate_koeder, -O3) -- Rot-Pflicht.")
    endif()
    # Fangnetz (2) am WURZEL-Ende (deckt im super-Embed auch die super-Ziele; im Standalone ist das
    # ein zweiter, idempotenter Lauf nach dem expliziten ce-baum-Lauf der Wurzel-CMakeLists).
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL comdare_vo31_optflag_gate_run "wurzel-ende")
endfunction()
