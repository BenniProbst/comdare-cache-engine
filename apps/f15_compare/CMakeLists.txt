# V41.F.6.1 R5.D — comdare-f15-compare CLI
#
# Schlankes, eigenstaendiges CLI um den F15-Messlauf-Treiber (Loader + IMeasurableWorkload +
# make_execution_result + multi_compare + summarize + Export). Header-only Mess-Bausteine
# (builder/commands/*), einzige Link-Abhaengigkeit ist der ModuleLoader + Boost::mp11.

add_executable(comdare_f15_compare_cli main.cpp)
set_target_properties(comdare_f15_compare_cli PROPERTIES
    OUTPUT_NAME "comdare-f15-compare")

target_include_directories(comdare_f15_compare_cli PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${CMAKE_BINARY_DIR}/generated")

target_link_libraries(comdare_f15_compare_cli PRIVATE
    comdare::anatomy_module_loader
    Boost::mp11)

target_compile_features(comdare_f15_compare_cli PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_f15_compare_cli)
endif()
