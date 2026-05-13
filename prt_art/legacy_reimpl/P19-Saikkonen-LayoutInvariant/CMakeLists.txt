# P19-Saikkonen-LayoutInvariant — Re-Implementation

add_library(comdare_legacy_p19_saikkonen_li INTERFACE)
add_library(comdare::legacy_p19_saikkonen_li ALIAS comdare_legacy_p19_saikkonen_li)
target_include_directories(comdare_legacy_p19_saikkonen_li INTERFACE include)
target_compile_features(comdare_legacy_p19_saikkonen_li INTERFACE cxx_std_23)

if(COMDARE_BUILD_TESTS)
    comdare_add_test(test_legacy_p19_saikkonen_li
        SOURCES tests/test_layout_invariant.cpp
        LIBRARIES comdare::legacy_p19_saikkonen_li)
endif()
