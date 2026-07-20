# cache_engine/builder/decision_lambda_trees — Sub-Komponente
# Hybrid-Rueckwaerts-Wahl-Entscheidungsbaum-SKELETON (Section 32-F8 / Section 49, 2026-07-20).
# ANGEBUNDEN: header-only INTERFACE-Target, das den curve_fit-Spline-Include (Break-Even = Spline-
# Schnittpunkte) + den Engine-Include-Root (cache_engine/measurement) an Konsumenten propagiert und das
# C++17-Minimum der Spline-/Break-Even-Mathematik setzt. decision_lambda_trees.hpp bruecke curve_fit ->
# Entscheidungsbaum. Reale Kurven bleiben #156-DATA-gated (synthetische Stuetzstellen im Vorbau/Test).

add_library(comdare_decision_lambda_trees INTERFACE)
target_include_directories(comdare_decision_lambda_trees INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../curve_fit
    ${PROJECT_SOURCE_DIR}/libs/cache_engine/include)
target_compile_features(comdare_decision_lambda_trees INTERFACE cxx_std_17)
