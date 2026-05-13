# P14-Samuel — Re-Implementation

add_library(comdare_legacy_p14_samuel INTERFACE)
add_library(comdare::legacy_p14_samuel ALIAS comdare_legacy_p14_samuel)
target_include_directories(comdare_legacy_p14_samuel INTERFACE include)
target_compile_features(comdare_legacy_p14_samuel INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p14_samuel
        SOURCES tests/test_config_table.cpp
        LIBRARIES comdare::legacy_p14_samuel)
endif()
