# Codegen - Phase 6.4
add_library(comdare_builder_codegen STATIC
    codegen.cpp
)
target_include_directories(comdare_builder_codegen PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(comdare_builder_codegen PUBLIC
    comdare_builder_xml_config_parser
)
target_compile_features(comdare_builder_codegen PUBLIC cxx_std_23)
