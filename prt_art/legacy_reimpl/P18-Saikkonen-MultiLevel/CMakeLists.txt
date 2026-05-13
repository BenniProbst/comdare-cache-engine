# P18-Saikkonen-MultiLevel — Re-Implementation

add_library(comdare_legacy_p18_saikkonen_ml INTERFACE)
add_library(comdare::legacy_p18_saikkonen_ml ALIAS comdare_legacy_p18_saikkonen_ml)
target_include_directories(comdare_legacy_p18_saikkonen_ml INTERFACE include)
target_compile_features(comdare_legacy_p18_saikkonen_ml INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p18_saikkonen_ml
        SOURCES tests/test_multi_level.cpp
        LIBRARIES comdare::legacy_p18_saikkonen_ml)
endif()
