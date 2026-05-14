# V26.B (2026-05-14) — A05-jemalloc Adapter-Skelett
add_library(comdare_adapter_a05_jemalloc INTERFACE)
add_library(comdare::adapter::a05_jemalloc ALIAS comdare_adapter_a05_jemalloc)
target_include_directories(comdare_adapter_a05_jemalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
