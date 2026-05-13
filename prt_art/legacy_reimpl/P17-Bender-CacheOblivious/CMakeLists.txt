# P17-Bender-CacheOblivious — Re-Implementation

add_library(comdare_legacy_p17_bender_co INTERFACE)
add_library(comdare::legacy_p17_bender_co ALIAS comdare_legacy_p17_bender_co)
target_include_directories(comdare_legacy_p17_bender_co INTERFACE include)
target_compile_features(comdare_legacy_p17_bender_co INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p17_bender_co
        SOURCES tests/test_veb_layout.cpp
        LIBRARIES comdare::legacy_p17_bender_co)
endif()
