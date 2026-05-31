# cmake/gtest_setup.cmake
# REV 5.3 Phase 6 - GoogleTest 1.15.2 via FetchContent (KEINE Git-Submodules)
#
# Memory-Direktive: KEIN Python in Build-Pipeline, KEINE Git-Submodules.
# GTest wird via FetchContent geholt; bei Offline-Build kann ein lokaler
# Tarball in cmake/third_party/googletest-1.15.2.tar.gz abgelegt werden.

include(FetchContent)
include(fetchcontent_stale_cleanup)
comdare_clean_stale_fetchcontent_subbuild(googletest)

# V41.P1-Gateway: vendored gtest relativ zu diesem .cmake (cmake/), nicht via CMAKE_SOURCE_DIR
# (= Superprojekt-Wurzel im add_subdirectory-Kontext).
set(_gtest_local "${CMAKE_CURRENT_LIST_DIR}/third_party/googletest-1.15.2.tar.gz")

if(EXISTS "${_gtest_local}")
    message(STATUS "GTest: lokaler Tarball gefunden: ${_gtest_local}")
    FetchContent_Declare(
        googletest
        URL "${_gtest_local}")
else()
    message(STATUS "GTest: lade v1.15.2 via FetchContent von GitHub Releases")
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
endif()

# Auf Windows die statische Runtime mit der Hauptbinary teilen
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# GTest-Komponenten konfigurieren (nur was wir brauchen)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Helper: lege Test-Executable mit GTest + ctest-Discovery an
function(COMDARE_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})

    COMDARE_set_cpp23(${name})
    COMDARE_set_default_warnings(${name})
    COMDARE_set_platform_defines(${name})

    target_link_libraries(${name} PRIVATE
        gtest
        gtest_main
        ${ARG_LIBRARIES})

    # ctest-Discovery
    # V41.E1: DISCOVERY_MODE PRE_TEST — Test-Enumeration zur ctest-Laufzeit statt POST_BUILD.
    # POST_BUILD (Default) fuehrt jede Test-.exe direkt nach dem Link aus; bei Tests mit DLL-Runtime-Deps
    # (z.B. R5G-AdHoc-DLL-Loader) oder im Sandbox-/CI-Lauf wirft MSBuild dabei MSB3073, obwohl Build+Test
    # spaeter passieren. PRE_TEST entkoppelt Discovery vom Build → kein Post-Build-Exe-Lauf, keine MSB3073.
    include(GoogleTest)
    gtest_discover_tests(${name}
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 30
        PROPERTIES TIMEOUT 60)
endfunction()
