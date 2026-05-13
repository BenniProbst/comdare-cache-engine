# P12-CSB-tree — Re-Implementation

add_library(comdare_legacy_p12_csb INTERFACE)
add_library(comdare::legacy_p12_csb ALIAS comdare_legacy_p12_csb)
target_include_directories(comdare_legacy_p12_csb INTERFACE include)
target_compile_features(comdare_legacy_p12_csb INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p12_csb
        SOURCES tests/test_csb.cpp
        LIBRARIES comdare::legacy_p12_csb)
endif()
