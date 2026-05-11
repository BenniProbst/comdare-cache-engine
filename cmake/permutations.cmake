# permutations.cmake — Trigger fuer Permutations-Codegen (F-EXTRA-5 NO-PYTHON, REVIDIERT durch H6)
#
# Memory-Direktive: KEIN Python in Build-Pipeline (Talos OS hat kein Python).
# Standardmaessig wird codegen.cmake via "cmake -P" aufgerufen.
# Optional via codegen.sh (POSIX) oder codegen.bat (Windows) — synchron gepflegt.

set(COMDARE_PERMUTATION_OUTPUT
    "${CMAKE_BINARY_DIR}/generated/permutations.cmake")

set(COMDARE_TARGET_ISA "auto" CACHE STRING
    "Target ISA fuer ConstraintFilter (auto = via PlatformProbe)")

set(COMDARE_PERMUTATION_CODEGEN_BACKEND "cmake" CACHE STRING
    "Codegen-Backend: cmake (Standard via cmake -P), sh, oder bat")
set_property(CACHE COMDARE_PERMUTATION_CODEGEN_BACKEND PROPERTY STRINGS cmake sh bat)

if(NOT COMDARE_BUILD_PERMUTATIONS)
    return()
endif()

if(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "cmake")
    set(_codegen_cmd
        ${CMAKE_COMMAND}
        "-DCOMDARE_TARGET_ISA=${COMDARE_TARGET_ISA}"
        "-DCOMDARE_OUTPUT=${COMDARE_PERMUTATION_OUTPUT}"
        -P "${CMAKE_SOURCE_DIR}/tools/permutation_codegen/codegen.cmake")
elseif(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "sh")
    set(_codegen_cmd
        sh "${CMAKE_SOURCE_DIR}/tools/permutation_codegen/codegen.sh"
        "--target-isa=${COMDARE_TARGET_ISA}"
        "--output=${COMDARE_PERMUTATION_OUTPUT}")
elseif(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "bat")
    set(_codegen_cmd
        cmd /c "${CMAKE_SOURCE_DIR}/tools/permutation_codegen/codegen.bat"
        "--target-isa=${COMDARE_TARGET_ISA}"
        "--output=${COMDARE_PERMUTATION_OUTPUT}")
else()
    message(FATAL_ERROR "Unbekanntes COMDARE_PERMUTATION_CODEGEN_BACKEND: ${COMDARE_PERMUTATION_CODEGEN_BACKEND}")
endif()

message(STATUS "Permutations-Codegen (Backend=${COMDARE_PERMUTATION_CODEGEN_BACKEND}) wird aufgerufen ...")
execute_process(
    COMMAND ${_codegen_cmd}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _codegen_result)

if(NOT _codegen_result EQUAL 0)
    message(FATAL_ERROR "Permutations-Codegen fehlgeschlagen (exit ${_codegen_result})")
endif()
