# experiment_driver — Library-Refactoring der Phase 1-7 (REV 7.6, Q4)

comdare_add_library(comdare_builder_experiment_driver PROJECT CACHE_ENGINE SOURCES
    experiment_driver.cpp
)
target_compile_features(comdare_builder_experiment_driver PUBLIC cxx_std_23)
target_include_directories(comdare_builder_experiment_driver PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_link_libraries(comdare_builder_experiment_driver PUBLIC
    comdare_builder_xml_config_parser
    comdare_builder_codegen
    comdare_builder_permutation_loop
    comdare_module_loader
    comdare::workload_generator
    comdare::experiment
)
add_library(comdare::builder::experiment_driver ALIAS comdare_builder_experiment_driver)
