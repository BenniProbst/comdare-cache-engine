# V26.B/V31.K4 (2026-05-14) — A08-scalloc Adapter
option(COMDARE_HAVE_SCALLOC "Activate ext/allocator/A08-scalloc/ (BSD-3)" OFF)
add_library(comdare_adapter_a08_scalloc INTERFACE)
add_library(comdare::adapter::a08_scalloc ALIAS comdare_adapter_a08_scalloc)
target_include_directories(comdare_adapter_a08_scalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a08_scalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_SCALLOC)
    target_compile_definitions(comdare_adapter_a08_scalloc INTERFACE COMDARE_HAVE_SCALLOC=1)
    target_include_directories(comdare_adapter_a08_scalloc INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/allocator/A08-scalloc/src")
endif()
