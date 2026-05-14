# V25.C (2026-05-14) — P25-Mahling Adapter-Skelett
add_library(comdare_adapter_p25_mahling INTERFACE)
add_library(comdare::adapter::p25_mahling ALIAS comdare_adapter_p25_mahling)
target_include_directories(comdare_adapter_p25_mahling INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
