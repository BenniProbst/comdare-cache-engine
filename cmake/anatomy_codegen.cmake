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
        "${CMAKE_SOURCE_DIR}/libs/cache_engine/builder/codegen/templates/anatomy_permutation_module.cpp.in")
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

    target_include_directories(${ARG_TARGET_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/libs/cache_engine"
        "${CMAKE_SOURCE_DIR}/libs/cache_engine/include"
        "${CMAKE_SOURCE_DIR}/libs/cache_engine/src"
        "${CMAKE_BINARY_DIR}/generated")

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
