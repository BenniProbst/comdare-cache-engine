# tools/permutation_codegen/codegen.cmake — Standard-Codegen (F-EXTRA-5 NO-PYTHON)
#
# Aufruf via:  cmake -DCOMDARE_TARGET_ISA=auto -DCOMDARE_OUTPUT=<path> -P codegen.cmake
#
# Aufgabe (Phase 4.B Skelett — Vollausbau in Phase 5+):
#   1. Liest Bausteine_Matrix (docs/bausteine/Bausteine_Matrix.txt)
#   2. Liest Flag_System (docs/architecture/Flag_System.txt)
#   3. Enumeriert gueltige Permutationen via ConstraintFilter
#   4. Generiert pro Permutation:
#        - Schalen-.cpp/.ccm (C++23-Modul-Wrapper)
#        - external_objects.cmake (Original-Compiler-Bausteine-Referenzen)
#        - CMake add_library(perm_<flags> ...) Eintrag
#   5. Output: build/generated/permutations.cmake
#
# Pendant-Skripte (synchron gepflegt):
#   - codegen.sh  (POSIX shell)
#   - codegen.bat (Windows cmd)

cmake_minimum_required(VERSION 3.28)

# ─────────────────────────────────────────────────────────────────────────────
# Eingabe-Parameter (per -D oder Umgebungsvariable)
# ─────────────────────────────────────────────────────────────────────────────
if(NOT DEFINED COMDARE_TARGET_ISA)
    set(COMDARE_TARGET_ISA "auto")
endif()
if(NOT DEFINED COMDARE_OUTPUT)
    if(DEFINED ENV{COMDARE_OUTPUT})
        set(COMDARE_OUTPUT "$ENV{COMDARE_OUTPUT}")
    else()
        message(FATAL_ERROR "COMDARE_OUTPUT erforderlich (Pfad zur generierten Datei)")
    endif()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Output-Verzeichnis sicherstellen
# ─────────────────────────────────────────────────────────────────────────────
get_filename_component(_out_dir "${COMDARE_OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

# ─────────────────────────────────────────────────────────────────────────────
# Skelett-Output (Phase 4.B — keine Permutationen, kommt in Phase 5+)
# ─────────────────────────────────────────────────────────────────────────────
file(WRITE "${COMDARE_OUTPUT}"
"# permutations.cmake — generiert von tools/permutation_codegen/codegen.cmake
# (Phase 4.B Skelett — keine Permutationen, kommt in Phase 5+)
# Target-ISA: ${COMDARE_TARGET_ISA}

message(STATUS \"Permutations-Codegen-Skelett (keine Permutationen registriert; ISA=${COMDARE_TARGET_ISA})\")
")

message(STATUS "OK (Skelett): ${COMDARE_OUTPUT} geschrieben.")
