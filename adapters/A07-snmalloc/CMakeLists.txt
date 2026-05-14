# V26.B/V31.K4 (2026-05-14) — A07-snmalloc Adapter
option(COMDARE_HAVE_SNMALLOC "Activate ext/A07-snmalloc/ (MIT)" OFF)
add_library(comdare_adapter_a07_snmalloc INTERFACE)
add_library(comdare::adapter::a07_snmalloc ALIAS comdare_adapter_a07_snmalloc)
target_include_directories(comdare_adapter_a07_snmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a07_snmalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_SNMALLOC)
    target_compile_definitions(comdare_adapter_a07_snmalloc INTERFACE COMDARE_HAVE_SNMALLOC=1)
    target_include_directories(comdare_adapter_a07_snmalloc INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/A07-snmalloc/src")
endif()
