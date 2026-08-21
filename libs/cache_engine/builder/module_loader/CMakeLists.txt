# cache_engine/builder/module_loader — Phase 7.2.D
# Plattform-agnostischer DLL/.so-Loader fuer Permutations-Module

comdare_add_library(comdare_module_loader PROJECT CACHE_ENGINE SOURCES
    module_loader.cpp
)
target_include_directories(comdare_module_loader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_compile_features(comdare_module_loader PUBLIC cxx_std_23)

# Plattform-spezifische Dependencies
if(UNIX AND NOT APPLE)
    target_link_libraries(comdare_module_loader PUBLIC dl)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_module_loader)
