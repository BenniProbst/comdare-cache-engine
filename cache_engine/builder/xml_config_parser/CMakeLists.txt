# XML Config Parser - Phase 6.4
add_library(comdare_builder_xml_config_parser STATIC
    xml_config_parser.cpp
)
target_include_directories(comdare_builder_xml_config_parser PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_compile_features(comdare_builder_xml_config_parser PUBLIC cxx_std_23)
