# V26.B/V31.K4 (2026-05-14) — A01-hoard Adapter
option(COMDARE_HAVE_HOARD "Activate ext/allocator/A01-hoard/ (Apache-2.0)" OFF)
add_library(comdare_adapter_a01_hoard INTERFACE)
add_library(comdare::adapter::a01_hoard ALIAS comdare_adapter_a01_hoard)
target_include_directories(comdare_adapter_a01_hoard INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a01_hoard INTERFACE cxx_std_23)
if(COMDARE_HAVE_HOARD)
    target_compile_definitions(comdare_adapter_a01_hoard INTERFACE COMDARE_HAVE_HOARD=1)
    target_include_directories(comdare_adapter_a01_hoard INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/allocator/A01-hoard/src/include")
endif()
