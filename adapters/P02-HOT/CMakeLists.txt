# V25.C (2026-05-14) — P02-HOT Adapter-Skelett
add_library(comdare_adapter_p02_hot INTERFACE)
add_library(comdare::adapter::p02_hot ALIAS comdare_adapter_p02_hot)
target_include_directories(comdare_adapter_p02_hot INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
