# benchmark_suite - Phase 6.6 (REV 7 §8)

add_library(comdare_benchmark_suite INTERFACE)
add_library(comdare::benchmark_suite ALIAS comdare_benchmark_suite)
target_include_directories(comdare_benchmark_suite INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(comdare_benchmark_suite INTERFACE cxx_std_23)
