# experiment - Phase 7 (REV 7 §9 Experiment-Loop)

add_library(comdare_experiment STATIC
    src/result_aggregator.cpp
)
add_library(comdare::experiment ALIAS comdare_experiment)
target_include_directories(comdare_experiment PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/cache_engine/include"
    "${CMAKE_SOURCE_DIR}/workload_generator/include"
    "${CMAKE_SOURCE_DIR}/benchmark_suite/include"
)
target_link_libraries(comdare_experiment PUBLIC
    comdare::workload_generator
    comdare::benchmark_suite
)
target_compile_features(comdare_experiment PUBLIC cxx_std_23)
