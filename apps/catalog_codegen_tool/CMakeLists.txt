# Limits-Entkopplung Vorstufe: XML-Profil -> generierter Source-Katalog-Header.

add_executable(comdare_catalog_codegen_cli main.cpp)
set_target_properties(comdare_catalog_codegen_cli PROPERTIES
    OUTPUT_NAME "comdare-catalog-codegen")

file(GLOB _ccg_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_ccg_axis_gen_dirs "")
foreach(_ccg_d ${_ccg_axis_src_dirs})
    if(IS_DIRECTORY "${_ccg_d}")
        file(RELATIVE_PATH _ccg_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_ccg_d}")
        list(APPEND _ccg_axis_gen_dirs "${CMAKE_BINARY_DIR}/generated/topics/${_ccg_rel}")
    endif()
endforeach()
file(GLOB _ccg_axes_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_ccg_d ${_ccg_axes_dirs})
    if(IS_DIRECTORY "${_ccg_d}")
        get_filename_component(_ccg_name "${_ccg_d}" NAME)
        list(APPEND _ccg_axis_gen_dirs "${CMAKE_BINARY_DIR}/generated/axes/${_ccg_name}")
    endif()
endforeach()

target_include_directories(comdare_catalog_codegen_cli PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${CMAKE_BINARY_DIR}/generated"
    ${_ccg_axis_gen_dirs})

target_link_libraries(comdare_catalog_codegen_cli PRIVATE
    Boost::mp11
    comdare_builder_xml_config_parser)
target_compile_features(comdare_catalog_codegen_cli PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_catalog_codegen_cli)
endif()

if(TARGET comdare_paper_codegen_all)
    add_dependencies(comdare_catalog_codegen_cli comdare_paper_codegen_all)
endif()
