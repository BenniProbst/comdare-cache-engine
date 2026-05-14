# V26.B (2026-05-14) — A07-snmalloc Adapter-Skelett
add_library(comdare_adapter_a07_snmalloc INTERFACE)
add_library(comdare::adapter::a07_snmalloc ALIAS comdare_adapter_a07_snmalloc)
target_include_directories(comdare_adapter_a07_snmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
