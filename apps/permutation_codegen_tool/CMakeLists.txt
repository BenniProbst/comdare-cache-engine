# #25 Teil B — permutation_codegen_tool CLI Executable (opt-in Backend 'cpp')

add_executable(comdare_permutation_codegen_cli main.cpp)
set_target_properties(comdare_permutation_codegen_cli PROPERTIES
    OUTPUT_NAME "comdare-permutation-codegen")

target_link_libraries(comdare_permutation_codegen_cli PRIVATE
    comdare::permutation_codegen_tool)

target_compile_features(comdare_permutation_codegen_cli PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_permutation_codegen_cli)
endif()
