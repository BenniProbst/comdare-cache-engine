# V41.F.6.1 R5.G — adhoc_emitter Module (Auto-Permutations-Skalierung)
#
# Schließt den letzten R5.G-Baustein: aus dem gemergten Permutations-Raum werden ALLE Permutationen
# automatisch zu Permutations-DLLs gebaut — ohne hand-geschriebene Composition-Liste (das war das
# F.5-comdare_codegen_anatomy_module_list-Muster). Stattdessen enumeriert der C++-Auto-Emitter
# (apps/adhoc_emitter) den Raum via for_each_composition_type und schreibt pro Permutation ein
# kompilierbares Modul-.cpp; dieses Modul wird hier per add_library als SHARED-DLL gebaut.
#
# 2-Pass-Build (analog anatomy_codegen_runner.cmake):
#   Pass 1: cmake -B build         -> Tool nicht gebaut, comdare_run_adhoc_emitter() skipt (Warning),
#                                      Tool-Target wird zur Build-Phase eingehaengt
#   Build:  cmake --build build --target comdare_adhoc_emitter_cli
#   Pass 2: cmake -B build         -> Tool gefunden, emittiert .cpp, comdare_build_adhoc_modules() baut DLLs
#   Build:  cmake --build build    -> alle Permutations-DLLs + Load-Test
#
# @doku docs/architecture/21_f6_plugin_controller_slot_merge_3stufen_bewiesen.md §5
# @task V41.F.6.1 R5.G

# ─────────────────────────────────────────────────────────────────────────────
# comdare_run_adhoc_emitter — ruft comdare-adhoc-emitter zur Configure-Time auf.
#   OUTPUT_DIR  <dir>        Ziel-Verzeichnis für die emittierten comdare_anatomy_perm_auto_*.cpp
#   [STATUS_OUT <var>]       "FOUND" | "SKIPPED" | "ERROR"
#   [FULL_COVERAGE]          --full-coverage an den Emitter durchreichen (1-wise-Stichprobe statt Pilot)
# ─────────────────────────────────────────────────────────────────────────────
function(comdare_run_adhoc_emitter)
    set(_options FULL_COVERAGE)
    set(_one_value OUTPUT_DIR STATUS_OUT)
    set(_multi_value)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    # Optionales --full-coverage-Argument (R5.D: 1-wise-Ueberdeckung über ALLE Achsen-Varianten).
    set(_emitter_extra_args "")
    if(ARG_FULL_COVERAGE)
        list(APPEND _emitter_extra_args "--full-coverage")
    endif()

    if(NOT ARG_OUTPUT_DIR)
        message(FATAL_ERROR "comdare_run_adhoc_emitter: OUTPUT_DIR required.")
    endif()

    macro(_ae_set_status_and_return _val)
        if(ARG_STATUS_OUT)
            set("${ARG_STATUS_OUT}" "${_val}" PARENT_SCOPE)
        endif()
        return()
    endmacro()

    # ─── Tool-Binary lokalisieren (build-tree-search, Cross-Plattform) ──
    set(_exe_name "comdare-adhoc-emitter")
    if(WIN32)
        set(_exe_name "${_exe_name}.exe")
    endif()
    set(_tool_dir "${CMAKE_BINARY_DIR}/apps/adhoc_emitter")
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
            "comdare_run_adhoc_emitter: tool not yet built.\n"
            "  Expected one of: ${_tool_candidates}\n"
            "  Run 'cmake --build <build-dir> --target comdare_adhoc_emitter_cli' "
            "and re-configure to enable R5.G Auto-Permutations-Skalierung.")
        _ae_set_status_and_return("SKIPPED")
    endif()

    file(MAKE_DIRECTORY "${ARG_OUTPUT_DIR}")
    message(STATUS "comdare_run_adhoc_emitter: running ${_tool_path} ${ARG_OUTPUT_DIR} ${_emitter_extra_args}")
    execute_process(
        COMMAND "${_tool_path}" "${ARG_OUTPUT_DIR}" ${_emitter_extra_args}
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE  _stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)

    if(NOT _rc EQUAL 0)
        message(WARNING
            "comdare_run_adhoc_emitter: tool returned rc=${_rc}\n"
            "  stdout: ${_stdout}\n  stderr: ${_stderr}")
        _ae_set_status_and_return("ERROR")
    endif()

    message(STATUS "comdare_run_adhoc_emitter: emitted modules into ${ARG_OUTPUT_DIR}\n"
        "  ${_stderr}")
    _ae_set_status_and_return("FOUND")
endfunction()


# ─────────────────────────────────────────────────────────────────────────────
# comdare_build_adhoc_modules — globt die emittierten comdare_anatomy_perm_auto_*.cpp
# in OUTPUT_DIR und baut jede als eigene SHARED-DLL (load_all-fähig im selben Verzeichnis).
#   OUTPUT_DIR    <dir>      Verzeichnis der emittierten .cpp (= RUNTIME_OUTPUT_DIRECTORY der DLLs)
#   PILOT_PREFIX  <p>        Target-Name-Prefix (Targets: <p>_0, <p>_1, ...)
#   AXIS_GEN_DIRS <dirs>     generierte axis_*_flags.hpp-Include-Dirs (COMDARE_ALL_AXIS_GENERATED_DIRS)
#   [TARGETS_OUT  <var>]     Liste der erzeugten Target-Namen
# ─────────────────────────────────────────────────────────────────────────────
function(comdare_build_adhoc_modules)
    set(_options)
    set(_one_value OUTPUT_DIR PILOT_PREFIX TARGETS_OUT)
    set(_multi_value AXIS_GEN_DIRS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg OUTPUT_DIR PILOT_PREFIX)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "comdare_build_adhoc_modules: ${_arg} required.")
        endif()
    endforeach()

    file(GLOB _emitted_cpps "${ARG_OUTPUT_DIR}/comdare_anatomy_perm_auto_*.cpp")
    list(SORT _emitted_cpps)
    list(LENGTH _emitted_cpps _n_emitted)
    if(_n_emitted EQUAL 0)
        message(WARNING
            "comdare_build_adhoc_modules: no emitted .cpp found in ${ARG_OUTPUT_DIR}.")
        if(ARG_TARGETS_OUT)
            set("${ARG_TARGETS_OUT}" "" PARENT_SCOPE)
        endif()
        return()
    endif()

    # V41.P1-Gateway: cache-engine-Wurzel robust via CMAKE_CURRENT_FUNCTION_LIST_DIR (Definitions-Datei
    # <ce>/cmake/, korrekt im Funktions-Aufruf; CMAKE_SOURCE_DIR = Superprojekt-Wurzel im subdir-Kontext).
    set(_ce_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    set(_target_list "")
    set(_index 0)
    foreach(_cpp ${_emitted_cpps})
        set(_target "${ARG_PILOT_PREFIX}_${_index}")
        add_library(${_target} SHARED "${_cpp}")
        set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS ${_target})
        # #188-4c-0b-R1: emitted modules can include Composition headers that
        # transitively include generated Paper-Original-Code wrappers.
        foreach(_pc
                comdare_paper_a04_mimalloc_codegen  comdare_paper_a05_jemalloc_codegen
                comdare_paper_a07_snmalloc_codegen  comdare_paper_a20_dlmalloc_codegen
                comdare_paper_a10_rpmalloc_codegen  comdare_paper_a11_lrmalloc_codegen
                comdare_paper_p01_art_codegen       comdare_paper_p02_hot_codegen
                comdare_paper_p05_start_codegen     comdare_paper_p07_wormhole_codegen
                comdare_paper_p10_surf_codegen      comdare_paper_p03_masstree_codegen
                comdare_paper_q01_concurrentqueue_codegen)
            if(TARGET ${_pc})
                add_dependencies(${_target} ${_pc})
            endif()
        endforeach()
        target_include_directories(${_target} PRIVATE
            "${_ce_root}/libs/cache_engine"
            "${_ce_root}/libs/cache_engine/include"
            "${_ce_root}/libs/cache_engine/src"
            "${CMAKE_BINARY_DIR}/generated"
            ${ARG_AXIS_GEN_DIRS})
        target_link_libraries(${_target} PRIVATE Boost::mp11)
        target_compile_features(${_target} PRIVATE cxx_std_23)
        target_compile_definitions(${_target} PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
        set_target_properties(${_target} PROPERTIES
            POSITION_INDEPENDENT_CODE                ON
            OUTPUT_NAME                              "comdare_anatomy_perm_auto_${_index}"
            RUNTIME_OUTPUT_DIRECTORY                 "${ARG_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${ARG_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${ARG_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY                 "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE         "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG           "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${ARG_OUTPUT_DIR}")
        if(COMMAND comdare_set_platform_defines)
            comdare_set_platform_defines(${_target})
        endif()
        list(APPEND _target_list "${_target}")
        math(EXPR _index "${_index} + 1")
    endforeach()

    if(ARG_TARGETS_OUT)
        set("${ARG_TARGETS_OUT}" "${_target_list}" PARENT_SCOPE)
    endif()
    message(STATUS
        "comdare_build_adhoc_modules: ${_n_emitted} auto-emittierte Permutations-DLLs "
        "(prefix=${ARG_PILOT_PREFIX}, output=${ARG_OUTPUT_DIR})")
endfunction()
