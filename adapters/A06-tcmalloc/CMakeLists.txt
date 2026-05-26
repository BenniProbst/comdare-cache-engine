# V26.B/V31.K4 (2026-05-14) — A06-tcmalloc Adapter
option(COMDARE_HAVE_TCMALLOC "Activate ext/allocator/A06-tcmalloc/ (Apache-2.0)" OFF)
add_library(comdare_adapter_a06_tcmalloc INTERFACE)
add_library(comdare::adapter::a06_tcmalloc ALIAS comdare_adapter_a06_tcmalloc)
target_include_directories(comdare_adapter_a06_tcmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a06_tcmalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_TCMALLOC)
    target_compile_definitions(comdare_adapter_a06_tcmalloc INTERFACE COMDARE_HAVE_TCMALLOC=1)
    target_include_directories(comdare_adapter_a06_tcmalloc INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/allocator/A06-tcmalloc/tcmalloc")
endif()
