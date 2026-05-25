# workload_generator - Phase 7.1 (REV 7 §7 Workload-Generation)

add_library(comdare_workload_generator STATIC
    src/workload_generator.cpp
)
add_library(comdare::workload_generator ALIAS comdare_workload_generator)
target_include_directories(comdare_workload_generator PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_compile_features(comdare_workload_generator PUBLIC cxx_std_23)
