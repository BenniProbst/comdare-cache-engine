# V41.F.6.1.R5.D.2 — anatomy_codegen Module
#
# Generiert pro AdHocComposition eine .cpp-Datei + Build-Target. Nutzt
# `configure_file` mit `anatomy_permutation_module.cpp.in`-Template.
#
# Usage:
#   include(cmake/anatomy_codegen.cmake)
#   comdare_codegen_anatomy_module(
#       TARGET_NAME         anatomy_perm_hot_static
#       COMPOSITION_TYPE    "::comdare::cache_engine::compositions::HotComposition"
#       COMPOSITION_HEADER  "compositions/hot_reference.hpp"
#       LIBRARY_TYPE        STATIC                                # SHARED | STATIC (Default: SHARED)
#       OUTPUT_DIR          "${CMAKE_BINARY_DIR}/generated_anatomy_modules"
#       FINGERPRINT         "hot_pilot"                           # optional (Default: SHA256-Prefix der COMPOSITION_TYPE)
#   )
#
# Build-Target:
#   - SHARED: liefert .so/.dll fuer R5.E ModuleLoader (dlopen/LoadLibrary)
#   - STATIC: liefert .a/.lib fuer In-Process Test-Verlinkung (R5.D.2 Pilot)
#
# Cross-Plattform:
#   - Linux/macOS: -fPIC + .so
#   - Windows:     .dll + .lib (Implib)
#
# @reference docs/architektur/14_achsen_komposition_organ_metapher.md §44 + §45
# @task V41.F.6.1.R5.D.2

function(comdare_codegen_anatomy_module)
    set(_options)
    set(_one_value
        TARGET_NAME
        COMPOSITION_TYPE
        COMPOSITION_HEADER
        OUTPUT_DIR
        LIBRARY_TYPE
        FINGERPRINT)
    set(_multi_value)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    # V41.P1-Gateway: cache-engine-Wurzel robust ueber CMAKE_CURRENT_FUNCTION_LIST_DIR (= Verzeichnis der
    # DEFINIERENDEN Datei <ce>/cmake/, korrekt auch im Funktions-Aufruf — CMAKE_CURRENT_LIST_DIR zeigte
    # sonst auf den Aufrufer, CMAKE_SOURCE_DIR auf die Superprojekt-Wurzel im add_subdirectory-Kontext).
    set(_ce_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")

    # ─── Pflicht-Argumente validieren ──────────────────────────────────────
    foreach(_arg TARGET_NAME COMPOSITION_TYPE COMPOSITION_HEADER OUTPUT_DIR)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR
                "comdare_codegen_anatomy_module: ${_arg} required.")
        endif()
    endforeach()

    # ─── Defaults ──────────────────────────────────────────────────────────
    if(NOT ARG_LIBRARY_TYPE)
        set(ARG_LIBRARY_TYPE SHARED)
    endif()
    if(NOT ARG_LIBRARY_TYPE STREQUAL "SHARED" AND
       NOT ARG_LIBRARY_TYPE STREQUAL "STATIC")
        message(FATAL_ERROR
            "comdare_codegen_anatomy_module: LIBRARY_TYPE must be SHARED or STATIC "
            "(got: ${ARG_LIBRARY_TYPE}).")
    endif()

    if(NOT ARG_FINGERPRINT)
        # Fingerprint = ersten 16 Hex-Chars des SHA256 ueber COMPOSITION_TYPE
        string(SHA256 _full_hash "${ARG_COMPOSITION_TYPE}")
        string(SUBSTRING "${_full_hash}" 0 16 ARG_FINGERPRINT)
    endif()

    # ─── Template-Variablen fuer configure_file ───────────────────────────
    set(COMDARE_COMPOSITION_TYPE        "${ARG_COMPOSITION_TYPE}")
    set(COMDARE_COMPOSITION_HEADER      "${ARG_COMPOSITION_HEADER}")
    set(COMDARE_COMPOSITION_FINGERPRINT "${ARG_FINGERPRINT}")
    string(TIMESTAMP COMDARE_CODEGEN_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

    # ─── Output-Pfade ─────────────────────────────────────────────────────
    set(_template_path
        "${_ce_root}/libs/cache_engine/builder/codegen/templates/anatomy_permutation_module.cpp.in")
    if(NOT EXISTS "${_template_path}")
        message(FATAL_ERROR
            "comdare_codegen_anatomy_module: Template not found at ${_template_path}")
    endif()

    set(_output_cpp "${ARG_OUTPUT_DIR}/anatomy_perm_${ARG_FINGERPRINT}.cpp")
    file(MAKE_DIRECTORY "${ARG_OUTPUT_DIR}")

    # ─── configure_file: Template → konkrete .cpp ────────────────────────
    configure_file(
        "${_template_path}"
        "${_output_cpp}"
        @ONLY)

    message(STATUS
        "comdare_codegen_anatomy_module: ${ARG_TARGET_NAME} "
        "(${ARG_LIBRARY_TYPE}, fp=${ARG_FINGERPRINT}) -> ${_output_cpp}")

    # ─── add_library mit generierter .cpp ────────────────────────────────
    add_library(${ARG_TARGET_NAME} ${ARG_LIBRARY_TYPE} "${_output_cpp}")
    # #278 (2026-07-06, Job 214698): der AnatomyModuleLoader filtert auf comdare_anatomy_perm_* (dokumentiertes
    # Pattern, anatomy_module_loader.cpp:173) — CMakes lib-Praefix brach den Vertrag auf Linux (alle Pilot-DLLs
    # hiessen libcomdare_...so -> load_all fand 0). PREFIX "" stellt das dokumentierte Pattern her.
    set_target_properties(${ARG_TARGET_NAME} PROPERTIES PREFIX "")
    set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS ${ARG_TARGET_NAME})
    # #188-4c-0b-R1: generated anatomy modules include Composition headers that
    # can transitively include generated Paper-Original-Code wrappers.
    foreach(_pc
            comdare_paper_a04_mimalloc_codegen  comdare_paper_a05_jemalloc_codegen
            comdare_paper_a07_snmalloc_codegen  comdare_paper_a20_dlmalloc_codegen
            comdare_paper_a10_rpmalloc_codegen  comdare_paper_a11_lrmalloc_codegen
            comdare_paper_p01_art_codegen       comdare_paper_p02_hot_codegen
            comdare_paper_p05_start_codegen     comdare_paper_p07_wormhole_codegen
            comdare_paper_p10_surf_codegen      comdare_paper_p03_masstree_codegen
            comdare_paper_q01_concurrentqueue_codegen)
        if(TARGET ${_pc})
            add_dependencies(${ARG_TARGET_NAME} ${_pc})
        endif()
    endforeach()

    # V41.F.6.1.R7.3-buildfix: per-Achsen generierte flags.hpp-Dirs (source-derived,
    # ordering-invariant — analog tests/unit). Ohne diese fanden generierte Anatomy-Module
    # (Pilots) axis_NN_*_flags.hpp nicht im Full-ALL-Build (Compositions ziehen alle Achsen).
    file(GLOB _cdg_axis_src_dirs LIST_DIRECTORIES true
        "${_ce_root}/libs/cache_engine/topics/*/axis_*")
    set(_cdg_axis_gen_dirs "")
    foreach(_cdg_d ${_cdg_axis_src_dirs})
        if(IS_DIRECTORY "${_cdg_d}")
            file(RELATIVE_PATH _cdg_rel "${_ce_root}/libs/cache_engine/topics" "${_cdg_d}")
            list(APPEND _cdg_axis_gen_dirs "${CMAKE_BINARY_DIR}/generated/topics/${_cdg_rel}")
        endif()
    endforeach()

    target_include_directories(${ARG_TARGET_NAME} PRIVATE
        "${_ce_root}/libs/cache_engine"
        "${_ce_root}/libs/cache_engine/include"
        "${_ce_root}/libs/cache_engine/src"
        "${CMAKE_BINARY_DIR}/generated"
        ${_cdg_axis_gen_dirs})

    # Build-Mode-Defines fuer ABI-Header:
    #   STATIC: PUBLIC COMDARE_ANATOMY_ABI_STATIC (Lib + Consumer brauchen kein dll*)
    #   SHARED: PRIVATE COMDARE_ANATOMY_MODULE_BUILD (Author-Side dllexport,
    #           Consumer-Side faellt auf dllimport zurueck — Default)
    if(ARG_LIBRARY_TYPE STREQUAL "STATIC")
        target_compile_definitions(${ARG_TARGET_NAME} PUBLIC
            COMDARE_ANATOMY_ABI_STATIC=1)
    else()
        target_compile_definitions(${ARG_TARGET_NAME} PRIVATE
            COMDARE_ANATOMY_MODULE_BUILD=1)
    endif()

    target_compile_features(${ARG_TARGET_NAME} PRIVATE cxx_std_23)

    # Position-Independent-Code (Pflicht fuer SHARED-Lib auf Linux/macOS)
    set_target_properties(${ARG_TARGET_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME "comdare_anatomy_perm_${ARG_FINGERPRINT}")

    # Plattform-Defines (NUMA/SIMD/etc. konsistent zu rest of project)
    if(COMMAND comdare_set_platform_defines)
        comdare_set_platform_defines(${ARG_TARGET_NAME})
    endif()

    # Pflicht-Dependencies (Boost.MP11 wird transitiv ueber Composition-Header
    # (z.B. observer_aggregate, pruefling_merge) benoetigt).
    if(TARGET Boost::mp11)
        target_link_libraries(${ARG_TARGET_NAME} PUBLIC Boost::mp11)
    endif()

    # ─── Exporte fuer Aufrufer ──────────────────────────────────────────
    set("${ARG_TARGET_NAME}_GENERATED_CPP" "${_output_cpp}" PARENT_SCOPE)
    set("${ARG_TARGET_NAME}_FINGERPRINT"   "${ARG_FINGERPRINT}" PARENT_SCOPE)
endfunction()


# ─────────────────────────────────────────────────────────────────────────────
# V41.F.6.1.R5.D.3 — comdare_codegen_anatomy_module_list
# ─────────────────────────────────────────────────────────────────────────────
#
# Multi-Permutation-Codegen: ruft comdare_codegen_anatomy_module() fuer eine
# Liste von Compositions auf. Jeder Listen-Eintrag ist ein "type|header"-Paar
# (Pipe-getrennt — Semikolon waere CMake-List-Separator-Konflikt).
#
# Generiert pro Eintrag ein eigenes Build-Target. TARGET_NAME = PILOT_PREFIX_<index>.
# Output-DLLs landen alle in OUTPUT_DIR (gemeinsam ladbar via
# AnatomyModuleLoader::load_all).
#
# Usage:
#   comdare_codegen_anatomy_module_list(
#       PILOT_PREFIX  "anatomy_codegen_multi_pilot"
#       OUTPUT_DIR    "${CMAKE_BINARY_DIR}/generated/anatomy_modules_multi"
#       LIBRARY_TYPE  SHARED
#       COMPOSITIONS
#           "::comdare::cache_engine::compositions::ArtComposition|compositions/art_reference.hpp"
#           "::comdare::cache_engine::compositions::StartComposition|compositions/start_reference.hpp"
#           "::comdare::cache_engine::compositions::SurfComposition|compositions/surf_reference.hpp"
#       [TARGETS_OUT  <var-name>]     # Optional: Liste der erzeugten Target-Namen
#   )
#
# @reference docs/architektur/14_achsen_komposition_organ_metapher.md §47
# @task V41.F.6.1.R5.D.3
function(comdare_codegen_anatomy_module_list)
    set(_options)
    set(_one_value PILOT_PREFIX OUTPUT_DIR LIBRARY_TYPE TARGETS_OUT)
    set(_multi_value COMPOSITIONS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg PILOT_PREFIX OUTPUT_DIR COMPOSITIONS)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR
                "comdare_codegen_anatomy_module_list: ${_arg} required.")
        endif()
    endforeach()

    if(NOT ARG_LIBRARY_TYPE)
        set(ARG_LIBRARY_TYPE SHARED)
    endif()

    set(_target_list "")
    set(_index 0)
    foreach(_entry IN LISTS ARG_COMPOSITIONS)
        # Pipe-Split: "type|header"
        string(REPLACE "|" ";" _parts "${_entry}")
        list(LENGTH _parts _parts_len)
        if(NOT _parts_len EQUAL 2)
            message(FATAL_ERROR
                "comdare_codegen_anatomy_module_list: COMPOSITIONS entry "
                "must be 'type|header' (got: ${_entry}).")
        endif()
        list(GET _parts 0 _type)
        list(GET _parts 1 _header)

        set(_target "${ARG_PILOT_PREFIX}_${_index}")
        # Fingerprint = Pilot-Prefix-Index fuer stabile DLL-Namen
        # (statt SHA256-Default damit DLL-Namen lesbar bleiben fuer load_all-Pattern)
        set(_fingerprint "${ARG_PILOT_PREFIX}_${_index}")

        comdare_codegen_anatomy_module(
            TARGET_NAME         "${_target}"
            COMPOSITION_TYPE    "${_type}"
            COMPOSITION_HEADER  "${_header}"
            LIBRARY_TYPE        "${ARG_LIBRARY_TYPE}"
            OUTPUT_DIR          "${ARG_OUTPUT_DIR}"
            FINGERPRINT         "${_fingerprint}")

        # Alle DLLs in dasselbe RUNTIME_OUTPUT_DIRECTORY damit load_all sie
        # in einem Verzeichnis findet (default: tests/unit/Release/ je nach Generator).
        # Wir setzen explicit damit AnatomyModuleLoader::load_all sie alle dort sieht.
        set_target_properties("${_target}" PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY                  "${ARG_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE          "${ARG_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG            "${ARG_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO   "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY                  "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE          "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG            "${ARG_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO   "${ARG_OUTPUT_DIR}")

        list(APPEND _target_list "${_target}")
        math(EXPR _index "${_index} + 1")
    endforeach()

    if(ARG_TARGETS_OUT)
        set("${ARG_TARGETS_OUT}" "${_target_list}" PARENT_SCOPE)
    endif()

    message(STATUS
        "comdare_codegen_anatomy_module_list: generated ${_index} "
        "${ARG_LIBRARY_TYPE} targets (prefix=${ARG_PILOT_PREFIX}, output=${ARG_OUTPUT_DIR})")
endfunction()
