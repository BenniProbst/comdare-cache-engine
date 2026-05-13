# P16-Bender-TreeLayout — Re-Implementation

add_library(comdare_legacy_p16_bender_layout INTERFACE)
add_library(comdare::legacy_p16_bender_layout ALIAS comdare_legacy_p16_bender_layout)
target_include_directories(comdare_legacy_p16_bender_layout INTERFACE include)
target_compile_features(comdare_legacy_p16_bender_layout INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p16_bender_layout
        SOURCES tests/test_probability_layout.cpp
        LIBRARIES comdare::legacy_p16_bender_layout)
endif()
