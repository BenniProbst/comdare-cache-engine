# permutations.cmake — Trigger fuer Python-Codegen-Skript (F-EXTRA-5)
#
# Ruft das Python-Skript tools/permutation_codegen/codegen.py auf, das die
# gueltigen Modul-Permutationen aus Bausteine-Matrix + Flag-System enumeriert
# und CMake-Targets generiert.

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(COMDARE_PERMUTATION_CODEGEN
    "${CMAKE_SOURCE_DIR}/tools/permutation_codegen/codegen.py")
set(COMDARE_PERMUTATION_OUTPUT
    "${CMAKE_BINARY_DIR}/generated/permutations.cmake")

# ConstraintFilter-Achsen (kommen via -D-Flags in den Codegen)
set(COMDARE_TARGET_ISA "auto" CACHE STRING
    "Target ISA fuer ConstraintFilter (auto = via PlatformProbe)")

if(COMDARE_BUILD_PERMUTATIONS)
  message(STATUS "Permutations-Codegen wird aufgerufen ...")
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
            ${COMDARE_PERMUTATION_CODEGEN}
            --target-isa=${COMDARE_TARGET_ISA}
            --output=${COMDARE_PERMUTATION_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _codegen_result
  )
  if(NOT _codegen_result EQUAL 0)
    message(FATAL_ERROR "Permutations-Codegen fehlgeschlagen (exit ${_codegen_result})")
  endif()
endif()
