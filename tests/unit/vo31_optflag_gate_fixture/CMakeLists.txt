# =============================================================================
# VO3-1(b) OS-2-GATE-FIXTURE (Owner-Antwort OS-2, 26.08.2026): ein MINI-Projekt,
# das die ECHTE Haus-Flag-Strecke laedt (cmake/compiler_flags.cmake des Repos,
# Pfad kommt als -DCOMDARE_CE_CMAKE_DIR herein) und auf Bestellung genau EINEN
# Koeder pflanzt. Der Treiber (../vo31_optflag_gate_probe.cmake) konfiguriert
# dieses Projekt viermal und prueft: konform => Configure GRUEN mit Gate-GRUEN-
# Literal; jeder Koeder => Configure bricht FATAL mit Gate-ROT-Literal.
#
# T-1 ROT-ZUERST: solange compiler_flags.cmake das OS-2-Gate noch nicht laedt,
# konfigurieren die Koeder-Faelle GRUEN durch -- der Treiber-Test ist dann ROT
# (der Defekt "eigene Opt-Flags rutschen still durch" ist unentdeckt). Gruen
# wird er erst durch cmake/vo31_optflag_gate.cmake im selben Branch.
#
# KOEDER-BESTELLUNG (-DCOMDARE_VO31_PROBE_KOEDER=...):
#   ""            konformes Projekt (kein Koeder)
#   target_o3     Direktblock-Klasse: target_compile_options(... PRIVATE -O3)
#   subdir_shadow Vendor-/Subprojekt-Klasse: set(CMAKE_CXX_FLAGS_RELEASE -O3...)
#   source_prop   Vendor-TU-Klasse: set_source_files_properties(COMPILE_OPTIONS)
# =============================================================================
cmake_minimum_required(VERSION 3.25)
project(vo31gatefixture LANGUAGES CXX)

if(NOT DEFINED COMDARE_CE_CMAKE_DIR)
    message(FATAL_ERROR "FIXTURE: -DCOMDARE_CE_CMAKE_DIR=<repo>/cmake fehlt")
endif()
list(APPEND CMAKE_MODULE_PATH "${COMDARE_CE_CMAKE_DIR}")
include(compiler_flags)

set(COMDARE_VO31_PROBE_KOEDER "" CACHE STRING "Koeder-Bestellung des Gate-Treibers")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/konform.cpp" "int vo31_konform() { return 2; }\n")
add_library(fixture_konform STATIC "${CMAKE_CURRENT_BINARY_DIR}/gen/konform.cpp")

if(COMDARE_VO31_PROBE_KOEDER STREQUAL "target_o3")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_t.cpp" "int vo31_koeder_t() { return 3; }\n")
    add_library(koeder_target STATIC "${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_t.cpp")
    target_compile_options(koeder_target PRIVATE -O3)
elseif(COMDARE_VO31_PROBE_KOEDER STREQUAL "subdir_shadow")
    add_subdirectory(shadow)
elseif(COMDARE_VO31_PROBE_KOEDER STREQUAL "source_prop")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_s.cpp" "int vo31_koeder_s() { return 3; }\n")
    add_library(koeder_source STATIC "${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_s.cpp")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_s.cpp"
        PROPERTIES COMPILE_OPTIONS "-O3")
elseif(NOT COMDARE_VO31_PROBE_KOEDER STREQUAL "")
    message(FATAL_ERROR "FIXTURE: unbekannte Koeder-Bestellung '${COMDARE_VO31_PROBE_KOEDER}'")
endif()
