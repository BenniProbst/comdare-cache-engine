# V26.B/V31.K4 (2026-05-14) — A10-rpmalloc Adapter
option(COMDARE_HAVE_RPMALLOC "Activate ext/A10-rpmalloc/ (Public Domain / MIT)" OFF)
add_library(comdare_adapter_a10_rpmalloc INTERFACE)
add_library(comdare::adapter::a10_rpmalloc ALIAS comdare_adapter_a10_rpmalloc)
target_include_directories(comdare_adapter_a10_rpmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a10_rpmalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_RPMALLOC)
    target_compile_definitions(comdare_adapter_a10_rpmalloc INTERFACE COMDARE_HAVE_RPMALLOC=1)
    target_include_directories(comdare_adapter_a10_rpmalloc INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/A10-rpmalloc/rpmalloc")
endif()
