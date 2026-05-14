# V25.C (2026-05-14) — P10-SuRF Adapter-Skelett
add_library(comdare_adapter_p10_surf INTERFACE)
add_library(comdare::adapter::p10_surf ALIAS comdare_adapter_p10_surf)
target_include_directories(comdare_adapter_p10_surf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
