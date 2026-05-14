# V25.C (2026-05-14) — P07-Wormhole Adapter-Skelett
add_library(comdare_adapter_p07_wormhole INTERFACE)
add_library(comdare::adapter::p07_wormhole ALIAS comdare_adapter_p07_wormhole)
target_include_directories(comdare_adapter_p07_wormhole INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
