# V41.F.6.1.R5.F — anatomy_codegen_tool CLI Executable

add_executable(comdare_anatomy_codegen_cli main.cpp)
set_target_properties(comdare_anatomy_codegen_cli PROPERTIES
    OUTPUT_NAME "comdare-anatomy-codegen-tool")

target_link_libraries(comdare_anatomy_codegen_cli PRIVATE
    comdare::anatomy_codegen_tool)

target_compile_features(comdare_anatomy_codegen_cli PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_anatomy_codegen_cli)
endif()
