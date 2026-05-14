# V25.C (2026-05-14) — P05-START Adapter-Skelett
add_library(comdare_adapter_p05_start INTERFACE)
add_library(comdare::adapter::p05_start ALIAS comdare_adapter_p05_start)
target_include_directories(comdare_adapter_p05_start INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
