# V41.F.6.1.R6.A — workload_driver Library-Target

comdare_add_library(comdare_workload_driver PROJECT CACHE_ENGINE SOURCES
    workload_generator.cpp)

add_library(comdare::workload_driver ALIAS comdare_workload_driver)

target_include_directories(comdare_workload_driver PUBLIC
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

target_compile_features(comdare_workload_driver PUBLIC cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_workload_driver)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_workload_driver)
