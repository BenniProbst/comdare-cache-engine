# V26.B (2026-05-14) — A20-dlmalloc Adapter-Skelett
add_library(comdare_adapter_a20_dlmalloc INTERFACE)
add_library(comdare::adapter::a20_dlmalloc ALIAS comdare_adapter_a20_dlmalloc)
target_include_directories(comdare_adapter_a20_dlmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
