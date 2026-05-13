# P22-Chen-Fractal — Re-Implementation

add_library(comdare_legacy_p22_fractal INTERFACE)
add_library(comdare::legacy_p22_fractal ALIAS comdare_legacy_p22_fractal)
target_include_directories(comdare_legacy_p22_fractal INTERFACE include)
target_compile_features(comdare_legacy_p22_fractal INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p22_fractal
        SOURCES tests/test_fractal.cpp
        LIBRARIES comdare::legacy_p22_fractal)
endif()
