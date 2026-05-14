# V26.B (2026-05-14) — A04-mimalloc Adapter-Skelett
add_library(comdare_adapter_a04_mimalloc INTERFACE)
add_library(comdare::adapter::a04_mimalloc ALIAS comdare_adapter_a04_mimalloc)
target_include_directories(comdare_adapter_a04_mimalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
