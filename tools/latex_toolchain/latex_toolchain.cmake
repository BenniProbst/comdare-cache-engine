# latex_toolchain.cmake — Habich-Direktive H3 (Aufgabe #102)
# Dynamische Diagramm-Compile + LaTeX-Build (NO PYTHON, F-EXTRA-5 konform).
#
# Pipeline:
#   1. Sammelt alle .drawio-Dateien aus diagrams/
#   2. Konvertiert sie zu .pdf via drawio-cli (NPM) — Skip falls nicht verfuegbar
#   3. Sammelt alle .tex-Anhang-Dateien (aus tools/latex_anhang)
#   4. Ruft pdflatex/lualatex/xelatex auf
#   5. Resultat: Diplomarbeit.pdf
#
# Aufruf:
#   cmake -DCOMDARE_THESIS_SOURCE=<dir> -DCOMDARE_THESIS_OUTPUT=<dir> \
#          -P latex_toolchain.cmake

cmake_minimum_required(VERSION 3.28)

# ─────────────────────────────────────────────────────────────────────────────
# Eingaben (per -D oder Defaults)
# ─────────────────────────────────────────────────────────────────────────────
if(NOT DEFINED COMDARE_THESIS_SOURCE)
    set(COMDARE_THESIS_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../docs/thesis")
endif()
if(NOT DEFINED COMDARE_THESIS_OUTPUT)
    set(COMDARE_THESIS_OUTPUT "${CMAKE_BINARY_DIR}/thesis-build")
endif()
if(NOT DEFINED COMDARE_THESIS_MAIN_TEX)
    set(COMDARE_THESIS_MAIN_TEX "main.tex")
endif()
if(NOT DEFINED COMDARE_LATEX_ENGINE)
    set(COMDARE_LATEX_ENGINE "pdflatex")
endif()

message(STATUS "==============================================================")
message(STATUS "comdare LaTeX-Toolchain (Habich H3)")
message(STATUS "  Source        : ${COMDARE_THESIS_SOURCE}")
message(STATUS "  Output        : ${COMDARE_THESIS_OUTPUT}")
message(STATUS "  Main-TeX      : ${COMDARE_THESIS_MAIN_TEX}")
message(STATUS "  LaTeX-Engine  : ${COMDARE_LATEX_ENGINE}")
message(STATUS "==============================================================")

file(MAKE_DIRECTORY "${COMDARE_THESIS_OUTPUT}")

# ─────────────────────────────────────────────────────────────────────────────
# Phase 1: drawio -> pdf
# ─────────────────────────────────────────────────────────────────────────────
find_program(DRAWIO_CLI drawio)
if(DRAWIO_CLI)
    message(STATUS "drawio-cli gefunden: ${DRAWIO_CLI}")
    file(GLOB_RECURSE _drawio_files "${COMDARE_THESIS_SOURCE}/diagrams/*.drawio")
    foreach(_drawio ${_drawio_files})
        get_filename_component(_basename "${_drawio}" NAME_WE)
        set(_pdf "${COMDARE_THESIS_OUTPUT}/diagrams/${_basename}.pdf")
        message(STATUS "  Convert: ${_basename}.drawio -> ${_basename}.pdf")
        file(MAKE_DIRECTORY "${COMDARE_THESIS_OUTPUT}/diagrams")
        execute_process(
            COMMAND "${DRAWIO_CLI}" --export --format pdf --output "${_pdf}" "${_drawio}"
            RESULT_VARIABLE _rc
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT _rc EQUAL 0)
            message(WARNING "drawio-cli failed for ${_drawio} (rc=${_rc})")
        endif()
    endforeach()
else()
    message(WARNING "drawio-cli not found — diagrams will NOT be compiled. "
                    "Install via: npm install -g @drawio/drawio-desktop-cli")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Phase 2: LaTeX-Anhang-Tabellen sammeln (generiert von tools/latex_anhang)
# ─────────────────────────────────────────────────────────────────────────────
file(GLOB _anhang_tex_files "${COMDARE_THESIS_SOURCE}/anhang/*.tex")
if(_anhang_tex_files)
    file(MAKE_DIRECTORY "${COMDARE_THESIS_OUTPUT}/anhang")
    foreach(_tex ${_anhang_tex_files})
        get_filename_component(_name "${_tex}" NAME)
        configure_file("${_tex}" "${COMDARE_THESIS_OUTPUT}/anhang/${_name}" COPYONLY)
    endforeach()
    message(STATUS "  Copied ${_anhang_tex_files} TeX-Anhang files into build")
else()
    message(STATUS "  No anhang/*.tex files found in source")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Phase 3: pdflatex/lualatex Aufruf
# ─────────────────────────────────────────────────────────────────────────────
find_program(LATEX_CMD ${COMDARE_LATEX_ENGINE})
if(NOT LATEX_CMD)
    message(WARNING "${COMDARE_LATEX_ENGINE} not found — install TeXLive or MikTeX.")
    return()
endif()
message(STATUS "LaTeX-Engine: ${LATEX_CMD}")

set(_main_tex "${COMDARE_THESIS_SOURCE}/${COMDARE_THESIS_MAIN_TEX}")
if(NOT EXISTS "${_main_tex}")
    message(WARNING "Main-TeX nicht vorhanden: ${_main_tex}. Skipping LaTeX run.")
    return()
endif()

# Zwei Durchlaeufe fuer korrekte References + Bibliography
foreach(_pass 1 2)
    message(STATUS "  Pass ${_pass}: ${COMDARE_LATEX_ENGINE} ${COMDARE_THESIS_MAIN_TEX}")
    execute_process(
        COMMAND "${LATEX_CMD}"
            -interaction=nonstopmode
            -output-directory=${COMDARE_THESIS_OUTPUT}
            "${_main_tex}"
        RESULT_VARIABLE _rc
        WORKING_DIRECTORY "${COMDARE_THESIS_SOURCE}")
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "${COMDARE_LATEX_ENGINE} failed (pass ${_pass}, rc=${_rc})")
    endif()
endforeach()

# Falls Bibliography vorhanden, bibtex aufrufen
find_program(BIBTEX_CMD bibtex)
if(BIBTEX_CMD AND EXISTS "${COMDARE_THESIS_SOURCE}/refs.bib")
    get_filename_component(_main_we "${COMDARE_THESIS_MAIN_TEX}" NAME_WE)
    message(STATUS "  Bibtex: ${_main_we}")
    execute_process(
        COMMAND "${BIBTEX_CMD}" "${_main_we}"
        WORKING_DIRECTORY "${COMDARE_THESIS_OUTPUT}"
        RESULT_VARIABLE _rc)
    # Re-run latex 2x nach bibtex
    foreach(_pass 1 2)
        execute_process(
            COMMAND "${LATEX_CMD}"
                -interaction=nonstopmode
                -output-directory=${COMDARE_THESIS_OUTPUT}
                "${_main_tex}"
            WORKING_DIRECTORY "${COMDARE_THESIS_SOURCE}")
    endforeach()
endif()

message(STATUS "==============================================================")
message(STATUS "LaTeX-Toolchain abgeschlossen.")
message(STATUS "Output: ${COMDARE_THESIS_OUTPUT}/${COMDARE_THESIS_MAIN_TEX}.pdf")
message(STATUS "==============================================================")
