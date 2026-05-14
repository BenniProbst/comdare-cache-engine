# V26.B (2026-05-14) — A06-tcmalloc Adapter-Skelett
add_library(comdare_adapter_a06_tcmalloc INTERFACE)
add_library(comdare::adapter::a06_tcmalloc ALIAS comdare_adapter_a06_tcmalloc)
target_include_directories(comdare_adapter_a06_tcmalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
