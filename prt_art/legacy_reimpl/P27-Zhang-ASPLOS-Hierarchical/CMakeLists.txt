# P27-Zhang-ASPLOS-Hierarchical — Re-Implementation

add_library(comdare_legacy_p27_zhang_asplos INTERFACE)
add_library(comdare::legacy_p27_zhang_asplos ALIAS comdare_legacy_p27_zhang_asplos)
target_include_directories(comdare_legacy_p27_zhang_asplos INTERFACE include)
target_compile_features(comdare_legacy_p27_zhang_asplos INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p27_zhang_asplos
        SOURCES tests/test_hierarchical.cpp
        LIBRARIES comdare::legacy_p27_zhang_asplos)
endif()
