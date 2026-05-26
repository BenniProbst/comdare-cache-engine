# V41.F.6.1.P2.A tools_cache Module
#
# On-demand fetch + build von Build-Tools (xxd, etc.). Cross-Platform 3 OS + 4 ISAs
# (User-Pflicht [[consteval-sha256-function-validation]]).
#
# Globaler Cache-Pfad: ${CMAKE_SOURCE_DIR}/tools_cache/<tool>/
# Marker-File: .build_complete.marker
#
# xxd-Quelle: vim-Repository (single C-File ~500 LoC, Vim-Lizenz / MIT-aequivalent).
# Beim Bauen wird System-CC (${CMAKE_C_COMPILER}) verwendet (kein Paper-Compiler!).
#
# @reference docs/architektur/13_paper_legacy_code_architektur.md §13.3 (xxd-Fallback bei MSVC #embed-Blocker)

set(COMDARE_TOOLS_CACHE_DIR "${CMAKE_SOURCE_DIR}/tools_cache"
    CACHE PATH "Cache directory for on-demand Build-Tool builds")

#
# Sicherstellen dass xxd verfuegbar ist (System oder cached-build).
#
# 1. Prueft System-PATH (find_program)
# 2. Falls nicht: clont C-Source aus vim-Repo + baut mit System-CC
# 3. Setzt COMDARE_XXD_BIN als Cache-Variable
function(comdare_ensure_xxd)
    set(_tool_dir   "${COMDARE_TOOLS_CACHE_DIR}/xxd")
    set(_marker     "${_tool_dir}/.build_complete.marker")
    set(_bin_dir    "${_tool_dir}/bin")
    set(_xxd_bin    "${_bin_dir}/xxd")
    if(WIN32)
        set(_xxd_bin "${_bin_dir}/xxd.exe")
    endif()

    if(EXISTS "${_marker}")
        message(STATUS "comdare tools-cache HIT: xxd at ${_xxd_bin}")
        set(COMDARE_XXD_BIN "${_xxd_bin}" CACHE FILEPATH "" FORCE)
        return()
    endif()

    # 1. Try System-xxd (POSIX hat es ueblicherweise, MSVC nicht)
    find_program(_system_xxd xxd)
    if(_system_xxd)
        message(STATUS "comdare tools-cache: using system xxd at ${_system_xxd}")
        file(MAKE_DIRECTORY "${_bin_dir}")
        file(CREATE_LINK "${_system_xxd}" "${_xxd_bin}" SYMBOLIC)
        file(TOUCH "${_marker}")
        set(COMDARE_XXD_BIN "${_system_xxd}" CACHE FILEPATH "" FORCE)
        return()
    endif()

    message(STATUS "comdare tools-cache MISS: xxd — fetching from vim/master")

    # 2. Fetch xxd C-Source from vim repo
    set(_xxd_url "https://raw.githubusercontent.com/vim/vim/master/src/xxd/xxd.c")
    set(_src_dir "${_tool_dir}/src")
    set(_xxd_src "${_src_dir}/xxd.c")

    file(MAKE_DIRECTORY "${_src_dir}")
    file(MAKE_DIRECTORY "${_bin_dir}")

    file(DOWNLOAD "${_xxd_url}" "${_xxd_src}"
         STATUS _dl_status
         SHOW_PROGRESS)
    list(GET _dl_status 0 _dl_rc)
    if(NOT _dl_rc EQUAL 0)
        list(GET _dl_status 1 _dl_msg)
        message(FATAL_ERROR "comdare tools-cache: xxd download FAILED: ${_dl_msg}")
    endif()

    # 3. Build with System-CC (NICHT Paper-Compiler!)
    if(NOT CMAKE_C_COMPILER)
        message(FATAL_ERROR "comdare tools-cache: CMAKE_C_COMPILER not set — xxd build needs system C compiler")
    endif()

    execute_process(
        COMMAND "${CMAKE_C_COMPILER}" "${_xxd_src}" -o "${_xxd_bin}"
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE  _stderr)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "comdare tools-cache: xxd build FAILED\nstdout:\n${_stdout}\nstderr:\n${_stderr}")
    endif()

    file(TOUCH "${_marker}")
    set(COMDARE_XXD_BIN "${_xxd_bin}" CACHE FILEPATH "" FORCE)

    message(STATUS "comdare tools-cache READY: xxd at ${_xxd_bin}")
endfunction()
