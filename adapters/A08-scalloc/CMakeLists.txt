# V26.B (2026-05-14) — A08-scalloc Adapter-Skelett
add_library(comdare_adapter_a08_scalloc INTERFACE)
add_library(comdare::adapter::a08_scalloc ALIAS comdare_adapter_a08_scalloc)
target_include_directories(comdare_adapter_a08_scalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
