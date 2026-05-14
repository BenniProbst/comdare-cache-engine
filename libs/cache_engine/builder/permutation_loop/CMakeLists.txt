# Permutation Loop - Phase 6.4
add_library(comdare_builder_permutation_loop STATIC
    permutation_loop.cpp
)
target_include_directories(comdare_builder_permutation_loop PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(comdare_builder_permutation_loop PUBLIC
    comdare_builder_xml_config_parser
)
target_compile_features(comdare_builder_permutation_loop PUBLIC cxx_std_23)
