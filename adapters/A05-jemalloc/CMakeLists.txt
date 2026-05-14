# V26.B/V31.K4 (2026-05-14) — A05-jemalloc Adapter
option(COMDARE_HAVE_JEMALLOC "Activate ext/A05-jemalloc/ (BSD-2)" OFF)
add_library(comdare_adapter_a05_jemalloc INTERFACE)
add_library(comdare::adapter::a05_jemalloc ALIAS comdare_adapter_a05_jemalloc)
target_include_directories(comdare_adapter_a05_jemalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a05_jemalloc INTERFACE cxx_std_23)
if(COMDARE_HAVE_JEMALLOC)
    target_compile_definitions(comdare_adapter_a05_jemalloc INTERFACE COMDARE_HAVE_JEMALLOC=1)
    target_include_directories(comdare_adapter_a05_jemalloc INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/A05-jemalloc/include")
endif()
