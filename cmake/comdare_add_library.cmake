# V41.E10 — STATIC/SHARED-Bibliotheks-Achse.
#
# Einheitlicher Wrapper um add_library(), der den Linkage-Typ der COMDARE-First-Party-Libs
# (STATIC default / SHARED) PRO LOGISCHEM PROJEKT über eine CMake-Option steuert — statt den
# Typ in jeder einzelnen CMakeLists.txt hart zu verdrahten.
#
# Steuerung (zwei Ebenen):
#   • Globaler Default:    COMDARE_BUILD_SHARED_LIBS            (OFF = STATIC ⇒ keine Regression)
#   • Pro-Projekt-Override: COMDARE_<PROJECT>_BUILD_SHARED_LIBS  (default = globaler Wert)
#     PROJECT-Schlüssel dieses Repos: CACHE_ENGINE · COMMON · EXECUTION_ENGINE · TEST_INFRA
#
# Verwendung:
#   comdare_add_library(<target> PROJECT <KEY> SOURCES <src...>)
#
# AUSSERHALB dieses Helpers bleiben bewusst:
#   • INTERFACE-Libs (header-only) — haben keinen Linkage-Typ (62 Stück im Repo).
#   • die per-Permutation-Anatomie-DLLs (comdare_build_adhoc_modules) — MÜSSEN SHARED sein.

if(NOT DEFINED COMDARE_BUILD_SHARED_LIBS)
    option(COMDARE_BUILD_SHARED_LIBS
        "Globaler Default-Linkage der COMDARE-First-Party-Libs als SHARED (OFF = STATIC)" OFF)
endif()

function(comdare_add_library target)
    set(_options)
    set(_one_value PROJECT)
    set(_multi_value SOURCES)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT ARG_PROJECT)
        message(FATAL_ERROR "comdare_add_library(${target}): PROJECT <KEY> erforderlich.")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "comdare_add_library(${target}): SOURCES erforderlich.")
    endif()

    # Pro-Projekt-Option (default = globaler COMDARE_BUILD_SHARED_LIBS-Wert; user-überschreibbar).
    set(_proj_var "COMDARE_${ARG_PROJECT}_BUILD_SHARED_LIBS")
    if(NOT DEFINED ${_proj_var})
        set(${_proj_var} "${COMDARE_BUILD_SHARED_LIBS}" CACHE BOOL
            "Linkage von Projekt ${ARG_PROJECT} als SHARED (default folgt COMDARE_BUILD_SHARED_LIBS)")
    endif()

    if(${${_proj_var}})
        set(_type SHARED)
    else()
        set(_type STATIC)
    endif()

    add_library(${target} ${_type} ${ARG_SOURCES})

    if(_type STREQUAL "SHARED")
        # MSVC: ohne explizite __declspec(dllexport)-Annotation ALLE Symbole exportieren, damit der
        # STATIC→SHARED-Toggle ohne Quell-Änderung linkt. PIC für ELF-SHARED (GCC/Clang).
        set_target_properties(${target} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            POSITION_INDEPENDENT_CODE  ON)
    endif()

    message(VERBOSE "comdare_add_library: ${target} (Projekt ${ARG_PROJECT}) ⇒ ${_type}")
endfunction()
