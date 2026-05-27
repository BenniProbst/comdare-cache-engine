# V41.F.6.1.R5.F — anatomy_codegen_tool Library-Target

add_library(comdare_anatomy_codegen_tool STATIC
    anatomy_codegen_tool.cpp)

add_library(comdare::anatomy_codegen_tool ALIAS comdare_anatomy_codegen_tool)

target_include_directories(comdare_anatomy_codegen_tool PUBLIC
    "${CMAKE_SOURCE_DIR}/libs/cache_engine"
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/include"
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")

target_compile_features(comdare_anatomy_codegen_tool PUBLIC cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_anatomy_codegen_tool)
endif()

# R5.H: Boost.MP11 transitiv via Composition-Headers (queuing/observer_aggregate)
if(TARGET Boost::mp11)
    target_link_libraries(comdare_anatomy_codegen_tool PUBLIC Boost::mp11)
endif()
