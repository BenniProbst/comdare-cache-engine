# V41.F.6.1.R5.F — anatomy_codegen_tool Library-Target

add_library(comdare_anatomy_codegen_tool STATIC
    anatomy_codegen_tool.cpp)

add_library(comdare::anatomy_codegen_tool ALIAS comdare_anatomy_codegen_tool)

target_include_directories(comdare_anatomy_codegen_tool PUBLIC
    "${CMAKE_SOURCE_DIR}/libs/cache_engine"
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/include")

target_compile_features(comdare_anatomy_codegen_tool PUBLIC cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_anatomy_codegen_tool)
endif()
