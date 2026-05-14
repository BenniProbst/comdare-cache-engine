# V25.C (2026-05-14) — P03-Masstree Adapter-Skelett
add_library(comdare_adapter_p03_masstree INTERFACE)
add_library(comdare::adapter::p03_masstree ALIAS comdare_adapter_p03_masstree)
target_include_directories(comdare_adapter_p03_masstree INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
