# V26.B/V31.K4 (2026-05-14) — A11-lrmalloc Adapter
option(COMDARE_HAVE_LRMALLOC "Activate ext/A11-lrmalloc/ (MIT)" OFF)
add_library(comdare_adapter_a11_lrmalloc INTERFACE)
add_library(comdare::adapter::a11_lrmalloc ALIAS comdare_adapter_a11_lrmalloc)
target_include_directories(comdare_adapter_a11_lrmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a11_lrmalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_LRMALLOC)
    target_compile_definitions(comdare_adapter_a11_lrmalloc INTERFACE COMDARE_HAVE_LRMALLOC=1)
    target_include_directories(comdare_adapter_a11_lrmalloc INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/A11-lrmalloc")
endif()
