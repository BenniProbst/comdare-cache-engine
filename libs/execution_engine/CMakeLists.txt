# execution_engine - Phase 7 (REV 7 §9 Experiment-Loop)
# REV 7.7 V30.D.1 (2026-05-14): migriert von experiment/ -> libs/execution_engine/
# (User-Direktive: keine domain/-Zwischenebene)

comdare_add_library(comdare_experiment PROJECT EXECUTION_ENGINE SOURCES
    src/result_aggregator.cpp
)
add_library(comdare::experiment ALIAS comdare_experiment)
target_include_directories(comdare_experiment PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_link_libraries(comdare_experiment PUBLIC
    comdare::workload_generator
    comdare::benchmark_suite
)
target_compile_features(comdare_experiment PUBLIC cxx_std_23)
