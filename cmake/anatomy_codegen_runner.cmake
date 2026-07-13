# V41.F.6.1.R5.I — anatomy_codegen_runner Module
#
# Ruft das bereits gebaute `comdare-anatomy-codegen-tool` (siehe
# apps/anatomy_codegen_tool/) zur CMake-Configure-Time via execute_process.
# Output landet in `${CMAKE_BINARY_DIR}/generated/...` und kann unmittelbar
# via include() in den aktuellen Configure-Pass eingelesen werden.
#
# 2-Pass-Build-Workflow (Tool ist eigenes CMake-Target im selben Repo):
#   Pass 1: cmake -B build
#     -> Tool noch nicht gebaut, Function gibt Warning, skipt Codegen,
#        Tool-Target wird zur Build-Phase eingehaengt
#   Build:  cmake --build build --target comdare_anatomy_codegen_cli
#   Pass 2: cmake -B build
#     -> Tool wird im build-tree gefunden, execute_process generiert Snippet
#     -> include(<snippet>) + comdare_codegen_anatomy_module_list() funktioniert
#   Build:  cmake --build build
#     -> Pilot-DLLs + Test werden gebaut
#
# Idempotenz: wenn Output-Snippet aktueller als Tool-Binary, skip.
#
# Usage:
#   include(anatomy_codegen_runner)
#   comdare_run_anatomy_codegen_tool(
#       OUTPUT       "${CMAKE_BINARY_DIR}/generated/r5i_perm_list.cmake"
#       NAMES        "art,hot,wormhole"      # optional, default = alle 11 (bekannte CE)
#       LIBRARY_TYPE SHARED                  # optional, default = SHARED
#       [NO_KNOWN]                           # optional: bekannte CE-Compositions weglassen
#       [EXTERNAL_COMPOSITIONS               # optional (F.5): Pruefling-Compositions
#           "prtart-demo|::comdare::prt_art::slots::PrtArtCompositionDemo|prt_art/slots/prt_art_composition_demo.hpp"]
#       [STATUS_OUT  <var-name>])            # optional, "FOUND"/"SKIPPED"/"ERROR"
#
# Pruefling-Stufe-2-Codegen (nur prt-art):
#   comdare_run_anatomy_codegen_tool(OUTPUT <pa.cmake> NO_KNOWN
#       EXTERNAL_COMPOSITIONS "prtart-demo|::comdare::...PrtArtCompositionDemo|prt_art/slots/...hpp")
#
# @doku docs/architektur/14_achsen_komposition_organ_metapher.md §51
# @task V41.F.6.1.R5.I

function(comdare_run_anatomy_codegen_tool)
    set(_options NO_KNOWN)
    set(_one_value OUTPUT NAMES LIBRARY_TYPE STATUS_OUT)
    set(_multi_value EXTERNAL_COMPOSITIONS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR
            "comdare_run_anatomy_codegen_tool: OUTPUT required.")
    endif()
    if(NOT ARG_LIBRARY_TYPE)
        set(ARG_LIBRARY_TYPE SHARED)
    endif()

    # Helper: setze STATUS_OUT wenn definiert, dann return
    macro(_set_status_and_return _val)
        if(ARG_STATUS_OUT)
            set("${ARG_STATUS_OUT}" "${_val}" PARENT_SCOPE)
        endif()
        return()
    endmacro()

    # ─── Tool-Binary lokalisieren (build-tree-search, Cross-Plattform) ──
    set(_exe_name "comdare-anatomy-codegen-tool")
    if(WIN32)
        set(_exe_name "${_exe_name}.exe")
    endif()

    # Muster-D/#14: PROJECT_BINARY_DIR — die apps/ landen im ce-Build-Baum (im super-Sub-Build
    # _cache_engine_external/apps), nicht in der Superprojekt-Build-Wurzel (CMAKE_BINARY_DIR).
    set(_tool_dir "${PROJECT_BINARY_DIR}/apps/anatomy_codegen_tool")
    set(_tool_candidates
        "${_tool_dir}/${_exe_name}"
        "${_tool_dir}/Release/${_exe_name}"
        "${_tool_dir}/Debug/${_exe_name}"
        "${_tool_dir}/RelWithDebInfo/${_exe_name}"
        "${_tool_dir}/MinSizeRel/${_exe_name}")

    set(_tool_path "")
    foreach(_cand ${_tool_candidates})
        if(EXISTS "${_cand}")
            set(_tool_path "${_cand}")
            break()
        endif()
    endforeach()

    if(NOT _tool_path)
        message(WARNING
            "comdare_run_anatomy_codegen_tool: tool not yet built.\n"
            "  Expected one of: ${_tool_candidates}\n"
            "  Run 'cmake --build <build-dir> --target comdare_anatomy_codegen_cli' "
            "and re-configure to enable R5.I Configure-Time Codegen.")
        _set_status_and_return("SKIPPED")
    endif()

    # ─── Idempotenz: wenn Output aktueller als Tool → HIT ───────────────
    if(EXISTS "${ARG_OUTPUT}")
        file(TIMESTAMP "${ARG_OUTPUT}" _output_ts "%s")
        file(TIMESTAMP "${_tool_path}" _tool_ts "%s")
        if(_output_ts GREATER _tool_ts OR _output_ts EQUAL _tool_ts)
            message(STATUS
                "comdare_run_anatomy_codegen_tool: HIT (idempotent) ${ARG_OUTPUT}")
            _set_status_and_return("FOUND")
        endif()
    endif()

    # ─── Output-Parent-Dir erstellen ────────────────────────────────────
    get_filename_component(_out_parent "${ARG_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_out_parent}")

    # ─── Tool aufrufen ──────────────────────────────────────────────────
    set(_args
        "--output"       "${ARG_OUTPUT}"
        "--library-type" "${ARG_LIBRARY_TYPE}")
    if(ARG_NAMES)
        list(APPEND _args "--names" "${ARG_NAMES}")
    endif()
    # F.5: Pruefling-Codegen — bekannte CE-Compositions unterdrücken + externe injizieren.
    if(ARG_NO_KNOWN)
        list(APPEND _args "--no-known")
    endif()
    foreach(_ext IN LISTS ARG_EXTERNAL_COMPOSITIONS)
        list(APPEND _args "--external-composition" "${_ext}")
    endforeach()

    message(STATUS "comdare_run_anatomy_codegen_tool: running ${_tool_path} ${_args}")
    execute_process(
        COMMAND "${_tool_path}" ${_args}
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE  _stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)

    if(NOT _rc EQUAL 0)
        message(WARNING
            "comdare_run_anatomy_codegen_tool: tool returned rc=${_rc}\n"
            "  stdout: ${_stdout}\n"
            "  stderr: ${_stderr}")
        _set_status_and_return("ERROR")
    endif()

    if(NOT EXISTS "${ARG_OUTPUT}")
        message(WARNING
            "comdare_run_anatomy_codegen_tool: tool succeeded but did not produce ${ARG_OUTPUT}")
        _set_status_and_return("ERROR")
    endif()

    message(STATUS
        "comdare_run_anatomy_codegen_tool: wrote ${ARG_OUTPUT}\n"
        "  stdout: ${_stdout}")

    _set_status_and_return("FOUND")
endfunction()
