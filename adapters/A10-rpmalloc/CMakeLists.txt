# V26.B (2026-05-14) — A10-rpmalloc Adapter-Skelett
add_library(comdare_adapter_a10_rpmalloc INTERFACE)
add_library(comdare::adapter::a10_rpmalloc ALIAS comdare_adapter_a10_rpmalloc)
target_include_directories(comdare_adapter_a10_rpmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
