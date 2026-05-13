# P13-Hankins — Re-Implementation

add_library(comdare_legacy_p13_hankins INTERFACE)
add_library(comdare::legacy_p13_hankins ALIAS comdare_legacy_p13_hankins)
target_include_directories(comdare_legacy_p13_hankins INTERFACE include)
target_compile_features(comdare_legacy_p13_hankins INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p13_hankins
        SOURCES tests/test_wider.cpp
        LIBRARIES comdare::legacy_p13_hankins)
endif()
