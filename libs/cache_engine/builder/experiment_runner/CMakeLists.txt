# Experiment Runner - Phase 6.4
comdare_add_library(comdare_builder_experiment_runner PROJECT CACHE_ENGINE SOURCES
    experiment_runner.cpp
)
target_include_directories(comdare_builder_experiment_runner PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_link_libraries(comdare_builder_experiment_runner PUBLIC
    comdare_builder_permutation_loop
)
target_compile_features(comdare_builder_experiment_runner PUBLIC cxx_std_23)
if(NOT WIN32)
    target_link_libraries(comdare_builder_experiment_runner PUBLIC dl)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_builder_experiment_runner)
