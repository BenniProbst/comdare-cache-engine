# V41.F.6.1.P1 Phase B — is_original_validator Pre-Build-Tool
# Standalone-Binary, wird von cmake/is_original_codegen.cmake zur Build-Time aufgerufen.

add_executable(is_original_validator main.cpp)
target_compile_features(is_original_validator PRIVATE cxx_std_23)
target_include_directories(is_original_validator PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
)
