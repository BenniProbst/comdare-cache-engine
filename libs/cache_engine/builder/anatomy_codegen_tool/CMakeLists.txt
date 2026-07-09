# V41.F.6.1.R5.F — anatomy_codegen_tool Library-Target

comdare_add_library(comdare_anatomy_codegen_tool PROJECT CACHE_ENGINE SOURCES
    anatomy_codegen_tool.cpp)

add_library(comdare::anatomy_codegen_tool ALIAS comdare_anatomy_codegen_tool)

# V41.F.6.1.R7.3-buildfix: per-Achsen generierte flags.hpp-Dirs (source-derived, ordering-invariant)
file(GLOB _act_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_act_axis_gen_dirs "")
foreach(_act_d ${_act_axis_src_dirs})
    if(IS_DIRECTORY "${_act_d}")
        file(RELATIVE_PATH _act_rel "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_act_d}")
        list(APPEND _act_axis_gen_dirs "${CMAKE_BINARY_DIR}/generated/topics/${_act_rel}")
    endif()
endforeach()

target_include_directories(comdare_anatomy_codegen_tool PUBLIC
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated"
    ${_act_axis_gen_dirs})

target_compile_features(comdare_anatomy_codegen_tool PUBLIC cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_anatomy_codegen_tool)
endif()

# R5.H: Boost.MP11 transitiv via Composition-Headers (queuing/observer_aggregate)
if(TARGET Boost::mp11)
    target_link_libraries(comdare_anatomy_codegen_tool PUBLIC Boost::mp11)
endif()
