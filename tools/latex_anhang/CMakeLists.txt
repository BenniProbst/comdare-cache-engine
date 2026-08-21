# tools/latex_anhang — LaTeX-Anhang fuer Diplomarbeit (Phase 8, REV 7 §8)

add_executable(comdare_latex_anhang main.cpp)
target_compile_features(comdare_latex_anhang PRIVATE cxx_std_23)
set_target_properties(comdare_latex_anhang PROPERTIES OUTPUT_NAME "latex_anhang")

if(COMDARE_BUILD_TESTS)
    add_library(comdare_latex_anhang_lib STATIC main.cpp)
    target_compile_definitions(comdare_latex_anhang_lib PRIVATE LATEX_ANHANG_TEST_NO_MAIN=1)
    target_compile_features(comdare_latex_anhang_lib PUBLIC cxx_std_23)
    target_include_directories(comdare_latex_anhang_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    add_library(comdare::latex_anhang_lib ALIAS comdare_latex_anhang_lib)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle tools). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_latex_anhang)
COMDARE_set_default_warnings(comdare_latex_anhang_lib)
