# P11-CSS-tree — Re-Implementation (Phase 6+)

add_library(comdare_legacy_p11_css INTERFACE)
add_library(comdare::legacy_p11_css ALIAS comdare_legacy_p11_css)
target_include_directories(comdare_legacy_p11_css INTERFACE include)
target_compile_features(comdare_legacy_p11_css INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p11_css
        SOURCES tests/test_css_node_page.cpp
        LIBRARIES comdare::legacy_p11_css)
endif()
