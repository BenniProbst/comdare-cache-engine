# =============================================================================
# VO3-1(b) OS-2-GATE-TREIBER (ctest -P Skript; Vorbild cli_smoke.cmake-Doktrin:
# BEIDE Haelften pruefen -- Exit-Code UND Literal, nie nur eines).
#
# Faehrt vier Configure-Laeufe der Fixture (../vo31_optflag_gate_fixture) und
# prueft je Fall Exit-Code UND Marker-Literal:
#   K0 konform        => rc==0  UND stdout traegt "OS-2-GATE GRUEN"
#   K1 target_o3      => rc!=0  UND Ausgabe traegt "OS-2-GATE ROT" + "koeder_target"
#   K2 subdir_shadow  => rc!=0  UND Ausgabe traegt "OS-2-GATE ROT" + "CMAKE_CXX_FLAGS_RELEASE"
#   K3 source_prop    => rc!=0  UND Ausgabe traegt "OS-2-GATE ROT" + "koeder_s.cpp"
# T-1 ROT-ZUERST: ohne das Gate (cmake/vo31_optflag_gate.cmake) konfigurieren
# K1-K3 GRUEN durch -> dieser Test faellt mit "KOEDER NICHT GEFANGEN" (Beweis
# bau/os2gate/rot-*.log). Erwartete -D Variablen: FIXTURE, CE_CMAKE_DIR,
# SCRATCH, GENERATOR, CXX.
# =============================================================================
foreach(_v FIXTURE CE_CMAKE_DIR SCRATCH GENERATOR CXX)
    if(NOT DEFINED ${_v})
        message(FATAL_ERROR "vo31_optflag_gate_probe: -D${_v}=... fehlt")
    endif()
endforeach()

set(_fails "")

function(_probe fall koeder erwartet_rot marker1 marker2)
    set(_bd "${SCRATCH}/${fall}")
    file(REMOVE_RECURSE "${_bd}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${FIXTURE}" -B "${_bd}" -G "${GENERATOR}"
                "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_CXX_COMPILER=${CXX}"
                "-DCOMDARE_CE_CMAKE_DIR=${CE_CMAKE_DIR}"
                "-DCOMDARE_VO31_PROBE_KOEDER=${koeder}"
        RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
    set(_ganz "${_out}\n${_err}")
    if(erwartet_rot)
        if(_rc EQUAL 0)
            string(APPEND _lokal "${fall}: KOEDER NICHT GEFANGEN (Configure rc=0, Gate fehlt oder stumpf);")
        else()
            foreach(_m IN ITEMS "${marker1}" "${marker2}")
                if(NOT _m STREQUAL "" AND NOT _ganz MATCHES "${_m}")
                    string(APPEND _lokal "${fall}: rc=${_rc} aber Marker '${_m}' fehlt in der Ausgabe;")
                endif()
            endforeach()
        endif()
    else()
        if(NOT _rc EQUAL 0)
            string(APPEND _lokal "${fall}: konformes Projekt bricht (rc=${_rc}): ${_err};")
        elseif(NOT _ganz MATCHES "${marker1}")
            string(APPEND _lokal "${fall}: rc=0 aber GRUEN-Literal '${marker1}' fehlt (Gate nicht geladen?);")
        endif()
    endif()
    if(DEFINED _lokal)
        message(STATUS "[vo31-gate-probe] ${fall} ROTBEFUND: ${_lokal}")
        set(_fails "${_fails}${_lokal}" PARENT_SCOPE)
    else()
        message(STATUS "[vo31-gate-probe] ${fall} OK (rc=${_rc}, Marker gedeckt)")
    endif()
endfunction()

_probe(k0_konform ""              FALSE "OS-2-GATE GRUEN" "")
_probe(k1_target  "target_o3"     TRUE  "OS-2-GATE ROT"   "koeder_target")
_probe(k2_subdir  "subdir_shadow" TRUE  "OS-2-GATE ROT"   "CMAKE_CXX_FLAGS_RELEASE")
_probe(k3_source  "source_prop"   TRUE  "OS-2-GATE ROT"   "koeder_s.cpp")

if(NOT _fails STREQUAL "")
    message(FATAL_ERROR "VO3-1(b) OS-2-GATE-PROBE ROT: ${_fails}")
endif()
message(STATUS "[vo31-gate-probe] 4/4 Faelle gedeckt (1x konform-gruen, 3x Koeder-FATAL mit Literal)")
