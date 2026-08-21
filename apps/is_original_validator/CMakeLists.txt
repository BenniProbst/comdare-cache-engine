# V41.F.6.1.P1 Phase B — is_original_validator Pre-Build-Tool
# Standalone-Binary, wird von cmake/is_original_codegen.cmake zur Build-Time aufgerufen.

add_executable(is_original_validator main.cpp)
target_compile_features(is_original_validator PRIVATE cxx_std_23)
target_include_directories(is_original_validator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
)

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle apps). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(is_original_validator)
