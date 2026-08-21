# XML Config Parser - Phase 6.4
comdare_add_library(comdare_builder_xml_config_parser PROJECT COMMON SOURCES
    xml_config_parser.cpp
)
target_include_directories(comdare_builder_xml_config_parser PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_compile_features(comdare_builder_xml_config_parser PUBLIC cxx_std_23)

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_builder_xml_config_parser)
