# P23-Khan-AdaptivePrefetch — Re-Implementation

add_library(comdare_legacy_p23_khan INTERFACE)
add_library(comdare::legacy_p23_khan ALIAS comdare_legacy_p23_khan)
target_include_directories(comdare_legacy_p23_khan INTERFACE include)
target_compile_features(comdare_legacy_p23_khan INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p23_khan
        SOURCES tests/test_adaptive.cpp
        LIBRARIES comdare::legacy_p23_khan)
endif()
