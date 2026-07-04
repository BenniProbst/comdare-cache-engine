# cmake/provenance.cmake
# Configure-time build provenance for host-side measurement export sidecars.

find_package(Git QUIET)

function(comdare_provenance_git_sha out_var repo_dir)
    set(_sha "unknown")
    if(GIT_FOUND AND GIT_EXECUTABLE AND IS_DIRECTORY "${repo_dir}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${repo_dir}" rev-parse HEAD
            RESULT_VARIABLE _git_result
            OUTPUT_VARIABLE _git_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_git_result EQUAL 0 AND NOT "${_git_output}" STREQUAL "")
            set(_sha "${_git_output}")
        endif()
    endif()
    set(${out_var} "${_sha}" PARENT_SCOPE)
endfunction()

function(comdare_provenance_escape out_var value)
    set(_escaped "${value}")
    string(REPLACE "\\" "\\\\" _escaped "${_escaped}")
    string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
    string(REPLACE "\n" "\\n" _escaped "${_escaped}")
    string(REPLACE "\r" "\\r" _escaped "${_escaped}")
    set(${out_var} "${_escaped}" PARENT_SCOPE)
endfunction()

comdare_provenance_git_sha(COMDARE_PROVENANCE_GIT_SHA_SUPER
    "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
comdare_provenance_git_sha(COMDARE_PROVENANCE_GIT_SHA_CACHE_ENGINE
    "${CMAKE_CURRENT_SOURCE_DIR}")
comdare_provenance_git_sha(COMDARE_PROVENANCE_GIT_SHA_PRT_ART
    "${CMAKE_CURRENT_SOURCE_DIR}/../comdare-prt-art")
comdare_provenance_git_sha(COMDARE_PROVENANCE_GIT_SHA_THESIS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../thesis/diplomarbeit")

set(COMDARE_PROVENANCE_CXX_FLAGS_RAW "${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _comdare_provenance_build_type)
    string(APPEND COMDARE_PROVENANCE_CXX_FLAGS_RAW
        " ${CMAKE_CXX_FLAGS_${_comdare_provenance_build_type}}")
endif()
string(STRIP "${COMDARE_PROVENANCE_CXX_FLAGS_RAW}" COMDARE_PROVENANCE_CXX_FLAGS_RAW)

comdare_provenance_escape(COMDARE_PROVENANCE_COMPILER_ID
    "${CMAKE_CXX_COMPILER_ID}")
comdare_provenance_escape(COMDARE_PROVENANCE_COMPILER_VERSION
    "${CMAKE_CXX_COMPILER_VERSION}")
comdare_provenance_escape(COMDARE_PROVENANCE_CXX_FLAGS
    "${COMDARE_PROVENANCE_CXX_FLAGS_RAW}")
comdare_provenance_escape(COMDARE_PROVENANCE_ISA_BUILT_FOR
    "${COMDARE_ISA_SUMMARY}")
comdare_provenance_escape(COMDARE_PROVENANCE_GIT_SHA_SUPER
    "${COMDARE_PROVENANCE_GIT_SHA_SUPER}")
comdare_provenance_escape(COMDARE_PROVENANCE_GIT_SHA_CACHE_ENGINE
    "${COMDARE_PROVENANCE_GIT_SHA_CACHE_ENGINE}")
comdare_provenance_escape(COMDARE_PROVENANCE_GIT_SHA_PRT_ART
    "${COMDARE_PROVENANCE_GIT_SHA_PRT_ART}")
comdare_provenance_escape(COMDARE_PROVENANCE_GIT_SHA_THESIS
    "${COMDARE_PROVENANCE_GIT_SHA_THESIS}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_provenance.hpp.in"
    "${CMAKE_BINARY_DIR}/generated/provenance/build_provenance.hpp"
    @ONLY)

include_directories("${CMAKE_BINARY_DIR}/generated")