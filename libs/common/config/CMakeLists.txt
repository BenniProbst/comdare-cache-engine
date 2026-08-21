# V30.D.3 (2026-05-14): engine_choice/ -> libs/common/config/
# Compile-Time Template-Wahl: StaticEngine vs CacheEngine
# (frueher: engine_choice/ Top-Level)

add_library(comdare_engine_choice INTERFACE)
add_library(comdare::common::config ALIAS comdare_engine_choice)
target_include_directories(comdare_engine_choice INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# template-only: static_engine.hpp + cache_engine.hpp werden in Phase 5 angelegt

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
