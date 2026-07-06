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

    # ctest-Registrierung — Pattern: "ein add_test je gtest-Binary" (Command-Objekt, GoF Command).
    #
    # HISTORIE & ROOT-CAUSE (warum NICHT gtest_discover_tests):
    #   V41.E1 nutzte gtest_discover_tests(DISCOVERY_MODE PRE_TEST). Beide gtest_discover_tests-Modi
    #   sind in dieser Toolchain-Kombination (CMake 4.2 + Visual-Studio-17-Multi-Config-Generator)
    #   unbrauchbar fuer eine reine `ctest -N`-Enumeration:
    #     * POST_BUILD fuehrt jede Test-.exe per add_custom_command bereits zur BUILD-Zeit aus
    #       (--gtest_list_tests). Bei DLL-Runtime-Dep-Tests (R5G-AdHoc-DLL-Loader) → MSB3073 im
    #       `cmake --build`. (= der Default-Build-Bruch, den E1 vermeiden wollte.)
    #     * PRE_TEST verschiebt den Exe-Lauf in die ctest-Laufzeit und erzeugt im Multi-Config-Pfad
    #       eine Defer-Datei `<target>[<n>]_include.cmake`, die `include("<...>_include-${CTEST_
    #       CONFIGURATION_TYPE}.cmake")` macht. Ein blankes `ctest -N` (ohne `-C`) laesst
    #       CTEST_CONFIGURATION_TYPE leer → CMake sucht `<...>_include-.cmake` → "could not find
    #       requested file" → Exit 8, 0 Tests enumeriert (das gemeldete Symptom).
    #     * Selbst MIT `-C Release` enumeriert PRE_TEST nicht: es ruft jede Binary mit
    #       `--gtest_list_tests --gtest_output=json:<dir-mit-leerzeichen>/...` auf; einzelne Tiere
    #       (z.B. test_v41_anatomy) HAENGEN unter dieser json-Discovery → DISCOVERY_TIMEOUT → Exit 8.
    #
    # SAUBERE LOESUNG (CMake-Primitiv `add_test`, kein Hack):
    #   Wir registrieren je Test-Binary GENAU EINEN ctest-Eintrag. Vorteile:
    #     * Enumeration zur CONFIGURE-Zeit, OHNE eine einzige Test-.exe auszufuehren → `ctest -N`
    #       (auch ohne `-C`) listet die gesamte Suite (>0), Exit 0; kein `[n]`-Defer, kein json-Hang.
    #     * Default-Build (`cmake --build`) unveraendert: kein POST_BUILD-Exe-Lauf → keine MSB3073.
    #     * Test-SEMANTIK unveraendert: `ctest` startet weiterhin die volle gtest-Binary; gtest_main
    #       fuehrt ALLE enthaltenen Faelle aus (gleiche Assertions, gleiche Exit-Bedeutung).
    #   Einziger Unterschied zu vorher: ctest-GRANULARITAET ist pro Binary statt pro gtest-Fall.
    #   Keine Stelle im Repo filtert ctest per einzelnem gtest-Fall (`ctest -R test_<case>`), daher
    #   ohne funktionalen Verlust. Pro-Fall-Aufschluesselung bleibt via `--gtest_filter` auf der
    #   Binary moeglich.
    add_test(
        NAME ${name}
        COMMAND ${name})
    # CI-2-Registry: Root-Ende konsumiert diese Targets als comdare_tests.
    # Erfasst auch conditional registrierte Post-Pass-2-Tests (r5i/f15), die
    # eine handgepflegte Liste verpassen wuerde.
    set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 60)
endfunction()
