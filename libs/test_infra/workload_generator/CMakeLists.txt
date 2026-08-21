# workload_generator - Phase 7.1 (REV 7 §7 Workload-Generation)

comdare_add_library(comdare_workload_generator PROJECT TEST_INFRA SOURCES
    src/workload_generator.cpp
)
add_library(comdare::workload_generator ALIAS comdare_workload_generator)
target_include_directories(comdare_workload_generator PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_compile_features(comdare_workload_generator PUBLIC cxx_std_23)

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG (Nachzug: dieses
# Verzeichnis lag ausserhalb des Wellen-Globs; TU-Zaehlung fand die Luecke).
COMDARE_set_default_warnings(comdare_workload_generator)
