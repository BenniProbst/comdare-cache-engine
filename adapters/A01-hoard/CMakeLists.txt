# V26.B (2026-05-14) — A01-hoard Adapter-Skelett
add_library(comdare_adapter_a01_hoard INTERFACE)
add_library(comdare::adapter::a01_hoard ALIAS comdare_adapter_a01_hoard)
target_include_directories(comdare_adapter_a01_hoard INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
