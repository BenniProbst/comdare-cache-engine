# comdare-succinct — eigene SDSL-Lite-Portierung (Aufgabe #105)

add_library(comdare_succinct INTERFACE)
add_library(comdare::succinct ALIAS comdare_succinct)
target_include_directories(comdare_succinct INTERFACE include)
target_compile_features(comdare_succinct INTERFACE cxx_std_23)

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
