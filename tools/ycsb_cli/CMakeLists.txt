# tools/ycsb_cli — YCSB-Generator-CLI (Phase C, 2026-05-13)

add_executable(comdare_ycsb_cli main.cpp)
target_link_libraries(comdare_ycsb_cli PRIVATE comdare::workload_generator)
target_compile_features(comdare_ycsb_cli PRIVATE cxx_std_23)
set_target_properties(comdare_ycsb_cli PROPERTIES OUTPUT_NAME "ycsb_cli")

if(COMDARE_BUILD_TESTS)
    # Test-Lib: re-compile main.cpp ohne main()
    add_library(comdare_ycsb_cli_lib STATIC main.cpp)
    target_link_libraries(comdare_ycsb_cli_lib PUBLIC comdare::workload_generator)
    target_compile_definitions(comdare_ycsb_cli_lib PRIVATE YCSB_CLI_TEST_NO_MAIN=1)
    target_compile_features(comdare_ycsb_cli_lib PUBLIC cxx_std_23)
    target_include_directories(comdare_ycsb_cli_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    add_library(comdare::ycsb_cli_lib ALIAS comdare_ycsb_cli_lib)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle tools). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_ycsb_cli)
if(TARGET comdare_ycsb_cli_lib) # existiert nur unter COMDARE_BUILD_TESTS (CI 16069 Job 382841)
    COMDARE_set_default_warnings(comdare_ycsb_cli_lib)
endif()
