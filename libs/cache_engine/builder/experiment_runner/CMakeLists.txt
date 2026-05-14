# Experiment Runner - Phase 6.4
add_library(comdare_builder_experiment_runner STATIC
    experiment_runner.cpp
)
target_include_directories(comdare_builder_experiment_runner PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/include"
)
target_link_libraries(comdare_builder_experiment_runner PUBLIC
    comdare_builder_permutation_loop
)
target_compile_features(comdare_builder_experiment_runner PUBLIC cxx_std_23)
if(NOT WIN32)
    target_link_libraries(comdare_builder_experiment_runner PUBLIC dl)
endif()
