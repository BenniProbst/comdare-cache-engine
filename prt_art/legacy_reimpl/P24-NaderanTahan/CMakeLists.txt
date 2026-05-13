# P24-NaderanTahan — Re-Implementation

add_library(comdare_legacy_p24_naderan INTERFACE)
add_library(comdare::legacy_p24_naderan ALIAS comdare_legacy_p24_naderan)
target_include_directories(comdare_legacy_p24_naderan INTERFACE include)
target_compile_features(comdare_legacy_p24_naderan INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p24_naderan
        SOURCES tests/test_useless.cpp
        LIBRARIES comdare::legacy_p24_naderan)
endif()
