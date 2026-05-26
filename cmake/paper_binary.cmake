# V41.F.6.1.P2.A paper_binary Module
#
# Compiliert Paper-Original-Code mit dem gecachten Paper-Compiler (via compiler_cache.cmake).
# Output: static library .a/.lib im ${CMAKE_BINARY_DIR}/paper_cache/<output>.
#
# Usage:
#   include(cmake/compiler_cache.cmake)
#   include(cmake/paper_binary.cmake)
#   comdare_ensure_compiler(NAME gcc-9.5 URL "..." SHA256 "...")
#   comdare_build_paper_binary(
#       PAPER     a04_mimalloc
#       COMPILER  gcc-9.5
#       SOURCES   "legacy_code/paper_a04_mimalloc/src/alloc.c"
#                 "legacy_code/paper_a04_mimalloc/src/heap.c"
#       FLAGS     "-O3" "-mavx2" "-DMI_DEBUG=0"
#       OUTPUT    "libpaper_a04_mimalloc.a")
#
# @reference docs/architektur/13_paper_legacy_code_architektur.md §6

#
# Compiliert eine Liste von Source-Files mit dem gecachten Paper-Compiler.
#
# Args:
#   PAPER      — Paper-ID (z.B. "a04_mimalloc") fuer Logging + Output-Naming
#   COMPILER   — Cache-Bezeichner aus comdare_ensure_compiler (z.B. "gcc-9.5")
#   SOURCES    — Liste der Source-Files (multi-value)
#   FLAGS      — Compile-Flags (multi-value)
#   OUTPUT     — Output-File-Name (im ${CMAKE_BINARY_DIR}/paper_cache/)
function(comdare_build_paper_binary)
    set(_options)
    set(_one_value PAPER COMPILER OUTPUT)
    set(_multi_value SOURCES FLAGS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg PAPER COMPILER OUTPUT)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "comdare_build_paper_binary: ${_arg} required")
        endif()
    endforeach()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "comdare_build_paper_binary: SOURCES required (at least 1 file)")
    endif()

    set(_compiler_bin "${COMDARE_COMPILER_${ARG_COMPILER}_BIN}")
    if(NOT _compiler_bin)
        message(FATAL_ERROR "comdare_build_paper_binary: Compiler ${ARG_COMPILER} not in cache. "
                            "Call comdare_ensure_compiler(NAME ${ARG_COMPILER} URL ... SHA256 ...) first.")
    endif()

    set(_paper_cache_dir "${CMAKE_BINARY_DIR}/paper_cache")
    file(MAKE_DIRECTORY "${_paper_cache_dir}")
    set(_paper_output "${_paper_cache_dir}/${ARG_OUTPUT}")

    # Idempotenz: wenn Output existiert, Cache-Hit (kein Re-Compile)
    if(EXISTS "${_paper_output}")
        message(STATUS "comdare paper-binary HIT: ${ARG_PAPER} at ${_paper_output}")
        return()
    endif()

    # Compiler-Pfad ermitteln (GCC-Stil .../bin/gcc, Clang .../bin/clang)
    set(_cc "")
    foreach(_candidate "gcc" "clang" "cc")
        if(EXISTS "${_compiler_bin}/${_candidate}")
            set(_cc "${_compiler_bin}/${_candidate}")
            break()
        endif()
        if(EXISTS "${_compiler_bin}/${_candidate}.exe")
            set(_cc "${_compiler_bin}/${_candidate}.exe")
            break()
        endif()
    endforeach()
    if(NOT _cc)
        message(FATAL_ERROR "comdare paper-binary: no compiler binary (gcc/clang/cc) at ${_compiler_bin}")
    endif()

    message(STATUS "comdare paper-binary: building ${ARG_PAPER} with ${_cc}")

    # Compile in einem Aufruf (sequentiell wie Original-Build-System)
    execute_process(
        COMMAND "${_cc}" ${ARG_FLAGS} -c ${ARG_SOURCES} -o "${_paper_output}"
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE  _stderr)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "comdare paper-binary BUILD FAILED: ${ARG_PAPER}\nstdout:\n${_stdout}\nstderr:\n${_stderr}")
    endif()

    message(STATUS "comdare paper-binary BUILT: ${ARG_PAPER} -> ${_paper_output}")
endfunction()
