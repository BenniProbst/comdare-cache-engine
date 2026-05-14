# V26.B (2026-05-14) — A11-lrmalloc Adapter-Skelett
add_library(comdare_adapter_a11_lrmalloc INTERFACE)
add_library(comdare::adapter::a11_lrmalloc ALIAS comdare_adapter_a11_lrmalloc)
target_include_directories(comdare_adapter_a11_lrmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
