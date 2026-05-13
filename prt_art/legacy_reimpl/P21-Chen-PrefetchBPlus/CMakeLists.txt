# P21-Chen-PrefetchBPlus — Re-Implementation

add_library(comdare_legacy_p21_chen_prefetch INTERFACE)
add_library(comdare::legacy_p21_chen_prefetch ALIAS comdare_legacy_p21_chen_prefetch)
target_include_directories(comdare_legacy_p21_chen_prefetch INTERFACE include)
target_compile_features(comdare_legacy_p21_chen_prefetch INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p21_chen_prefetch
        SOURCES tests/test_prefetch_bplus.cpp
        LIBRARIES comdare::legacy_p21_chen_prefetch)
endif()
