# V41.F.6.1.P2.A compiler_cache Module
#
# On-demand fetch + build von Paper-Compilern. Cross-Platform (User-Pflicht
# [[consteval-sha256-function-validation]]: 3 OS + 4 ISAs).
#
# Globaler Cache-Pfad: ${CMAKE_SOURCE_DIR}/compiler_cache/<name>/
# Marker-File: .build_complete.marker (Idempotenz)
#
# Usage:
#   include(cmake/compiler_cache.cmake)
#   comdare_ensure_compiler(
#       NAME       gcc-9.5
#       URL        "https://ftp.gnu.org/gnu/gcc/gcc-9.5.0/gcc-9.5.0.tar.xz"
#       SHA256     "27769f64ef1d4cd5e2c1c8af167d8c5a4f4c5b39c1234567890..."
#       BUILD_WITH "${CMAKE_C_COMPILER}")
#
# @reference docs/architektur/13_paper_legacy_code_architektur.md §5 + §12.3

set(COMDARE_COMPILER_CACHE_DIR "${CMAKE_SOURCE_DIR}/compiler_cache"
    CACHE PATH "Cache directory for on-demand Paper-Compiler builds")

#
# Sicherstellen dass ein konkreter Paper-Compiler installiert ist (bei Bedarf bauen).
#
# Args:
#   NAME              — Cache-Bezeichner, z.B. "gcc-9.5", "clang-12", "msvc-19.30"
#   URL               — Source-Archive-URL
#   SHA256            — Pflicht-Hash zur Integritaets-Pruefung
#   BUILD_WITH        — System-Compiler fuer den Bootstrap (Default ${CMAKE_C_COMPILER})
#   CONFIGURE_FLAGS   — optionale ./configure Flags (multi-value)
#
# Setzt nach Erfolg:
#   COMDARE_COMPILER_<NAME>_BIN     — Pfad zum bin/-Verzeichnis
#   COMDARE_COMPILER_<NAME>_PREFIX  — Pfad zum Install-Prefix
function(comdare_ensure_compiler)
    set(_options)
    set(_one_value NAME URL SHA256 BUILD_WITH)
    set(_multi_value CONFIGURE_FLAGS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg NAME URL SHA256)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "comdare_ensure_compiler: ${_arg} required")
        endif()
    endforeach()

    set(_compiler_dir "${COMDARE_COMPILER_CACHE_DIR}/${ARG_NAME}")
    set(_marker_file  "${_compiler_dir}/.build_complete.marker")

    if(EXISTS "${_marker_file}")
        message(STATUS "comdare compiler-cache HIT: ${ARG_NAME} at ${_compiler_dir}")
        set(COMDARE_COMPILER_${ARG_NAME}_BIN    "${_compiler_dir}/bin" CACHE PATH "" FORCE)
        set(COMDARE_COMPILER_${ARG_NAME}_PREFIX "${_compiler_dir}"     CACHE PATH "" FORCE)
        return()
    endif()

    message(STATUS "comdare compiler-cache MISS: ${ARG_NAME} — fetching from ${ARG_URL}")

    set(_source_archive "${_compiler_dir}/source.tar.xz")
    set(_build_dir      "${_compiler_dir}/build")

    # 1. Download
    file(DOWNLOAD "${ARG_URL}" "${_source_archive}"
         EXPECTED_HASH "SHA256=${ARG_SHA256}"
         SHOW_PROGRESS
         STATUS _dl_status)
    list(GET _dl_status 0 _dl_rc)
    if(NOT _dl_rc EQUAL 0)
        list(GET _dl_status 1 _dl_msg)
        message(FATAL_ERROR "comdare compiler-cache: download FAILED for ${ARG_NAME}: ${_dl_msg}")
    endif()

    # 2. Extract
    file(MAKE_DIRECTORY "${_build_dir}")
    file(ARCHIVE_EXTRACT INPUT "${_source_archive}" DESTINATION "${_build_dir}")

    if(UNIX)
        # 3a. UNIX: find source subdir + ./configure + make
        file(GLOB _children RELATIVE "${_build_dir}" "${_build_dir}/*")
        list(LENGTH _children _children_len)
        if(_children_len EQUAL 0)
            message(FATAL_ERROR "comdare compiler-cache: extracted archive empty (${ARG_NAME})")
        endif()
        list(GET _children 0 _src_subdir)
        set(_src_dir "${_build_dir}/${_src_subdir}")

        set(_build_with "${ARG_BUILD_WITH}")
        if(NOT _build_with)
            set(_build_with "${CMAKE_C_COMPILER}")
        endif()

        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env "CC=${_build_with}"
                "${_src_dir}/configure" "--prefix=${_compiler_dir}" ${ARG_CONFIGURE_FLAGS}
            WORKING_DIRECTORY "${_src_dir}"
            RESULT_VARIABLE   _cfg_rc)
        if(NOT _cfg_rc EQUAL 0)
            message(FATAL_ERROR "comdare compiler-cache: ./configure FAILED for ${ARG_NAME}")
        endif()

        execute_process(
            COMMAND make -j8 install
            WORKING_DIRECTORY "${_src_dir}"
            RESULT_VARIABLE   _make_rc)
        if(NOT _make_rc EQUAL 0)
            message(FATAL_ERROR "comdare compiler-cache: make install FAILED for ${ARG_NAME}")
        endif()
    else()
        # 3b. Windows: TODO P2.A.W — manual prebuilt-binary path or MinGW/MSVC-spez. build
        message(WARNING "comdare compiler-cache: Windows compiler-build pending (P2.A.W). "
                        "Use prebuilt binaries from vendor; place under ${_compiler_dir}/bin/ "
                        "then 'touch ${_marker_file}' to mark as ready.")
        return()
    endif()

    # 4. Marker
    file(TOUCH "${_marker_file}")
    set(COMDARE_COMPILER_${ARG_NAME}_BIN    "${_compiler_dir}/bin" CACHE PATH "" FORCE)
    set(COMDARE_COMPILER_${ARG_NAME}_PREFIX "${_compiler_dir}"     CACHE PATH "" FORCE)

    message(STATUS "comdare compiler-cache READY: ${ARG_NAME} at ${_compiler_dir}")
endfunction()
