# cmake/fetchcontent_stale_cleanup.cmake
# Auto-Cleanup fuer Stale-FetchContent-Subbuild-Caches.
#
# FetchContent legt `_deps/<name>-subbuild/CMakeCache.txt` mit absoluten Pfaden
# an. Wenn das Repo zwischen Configures umgezogen wurde (z.B. von
# Research/ nach Modules/), zeigt der alte Cache auf den falschen
# Source-Pfad und Configure failed mit:
#
#   CMake Error: The current CMakeCache.txt directory ... is different than
#   the directory ... where CMakeCache.txt was created.
#
# Diese Function prueft VOR FetchContent_MakeAvailable, ob der subbuild-Cache
# auf das aktuelle CMAKE_SOURCE_DIR zeigt. Bei Mismatch wird der subbuild-
# Ordner sicher bereinigt (betrifft NUR _deps/<name>-subbuild/, nicht
# User-Code).
#
# Usage:
#   include(fetchcontent_stale_cleanup)
#   comdare_clean_stale_fetchcontent_subbuild(googletest)
#   FetchContent_MakeAvailable(googletest)

function(comdare_clean_stale_fetchcontent_subbuild _name)
    set(_subbuild_dir "${CMAKE_BINARY_DIR}/_deps/${_name}-subbuild")
    set(_cache_file   "${_subbuild_dir}/CMakeCache.txt")
    if(NOT EXISTS "${_cache_file}")
        return()
    endif()
    file(STRINGS "${_cache_file}" _home_line REGEX "^CMAKE_HOME_DIRECTORY:[^=]+=(.*)$")
    if(NOT _home_line)
        return()
    endif()
    string(REGEX REPLACE "^CMAKE_HOME_DIRECTORY:[^=]+=(.*)$" "\\1" _cached_home "${_home_line}")
    # Normalisiere beide Pfade fuer Case-insensitive + Slash-Direction-Vergleich
    string(TOLOWER "${_cached_home}"    _cached_norm)
    string(TOLOWER "${CMAKE_SOURCE_DIR}" _current_norm)
    string(REPLACE "\\" "/" _cached_norm  "${_cached_norm}")
    string(REPLACE "\\" "/" _current_norm "${_current_norm}")
    if(NOT _cached_norm STREQUAL _current_norm)
        message(WARNING
            "FetchContent stale subbuild-Cache erkannt fuer '${_name}'.\n"
            "  Cached source : ${_cached_home}\n"
            "  Current source: ${CMAKE_SOURCE_DIR}\n"
            "  Auto-Bereinigung: loesche ${_subbuild_dir} (sicher, betrifft nur FetchContent-Subbuild).")
        file(REMOVE_RECURSE "${_subbuild_dir}")
    endif()
endfunction()
