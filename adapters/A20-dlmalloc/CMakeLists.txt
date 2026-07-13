# V26.B/V31.K4 (2026-05-14) — A20-dlmalloc Adapter
option(COMDARE_HAVE_DLMALLOC "Activate ext/allocator/A20-dlmalloc/ (Public Domain)" OFF)
add_library(comdare_adapter_a20_dlmalloc INTERFACE)
add_library(comdare::adapter::a20_dlmalloc ALIAS comdare_adapter_a20_dlmalloc)
target_include_directories(comdare_adapter_a20_dlmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a20_dlmalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_DLMALLOC)
    target_compile_definitions(comdare_adapter_a20_dlmalloc INTERFACE COMDARE_HAVE_DLMALLOC=1)
    target_include_directories(comdare_adapter_a20_dlmalloc INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/allocator/A20-dlmalloc")
endif()
