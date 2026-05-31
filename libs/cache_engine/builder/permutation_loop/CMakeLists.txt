# Permutation Loop - Phase 6.4
comdare_add_library(comdare_builder_permutation_loop PROJECT CACHE_ENGINE SOURCES
    permutation_loop.cpp
)
target_include_directories(comdare_builder_permutation_loop PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(comdare_builder_permutation_loop PUBLIC
    comdare_builder_xml_config_parser
)
target_compile_features(comdare_builder_permutation_loop PUBLIC cxx_std_23)
