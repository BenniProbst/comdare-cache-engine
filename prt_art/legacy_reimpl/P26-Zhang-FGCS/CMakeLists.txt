# P26-Zhang-FGCS — Re-Implementation

add_library(comdare_legacy_p26_zhang_fgcs INTERFACE)
add_library(comdare::legacy_p26_zhang_fgcs ALIAS comdare_legacy_p26_zhang_fgcs)
target_include_directories(comdare_legacy_p26_zhang_fgcs INTERFACE include)
target_compile_features(comdare_legacy_p26_zhang_fgcs INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p26_zhang_fgcs
        SOURCES tests/test_path_jumppointer.cpp
        LIBRARIES comdare::legacy_p26_zhang_fgcs)
endif()
