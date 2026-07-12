# #25 Teil B — permutation_codegen_tool Library-Target (C++23-Port von codegen.cmake)
#
# Duenne Bibliothek: reines Text-/Kreuzprodukt-Rendering, KEINE cache_engine-Header noetig
# (nur <filesystem>/<string>/<vector>). Der add_library-Wrapper folgt der STATIC/SHARED-Achse.

comdare_add_library(comdare_permutation_codegen_tool PROJECT CACHE_ENGINE SOURCES
    permutation_codegen_tool.cpp)

add_library(comdare::permutation_codegen_tool ALIAS comdare_permutation_codegen_tool)

target_include_directories(comdare_permutation_codegen_tool PUBLIC
    "${CMAKE_SOURCE_DIR}/libs/cache_engine")

target_compile_features(comdare_permutation_codegen_tool PUBLIC cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_permutation_codegen_tool)
endif()
